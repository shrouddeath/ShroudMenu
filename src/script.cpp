// ShroudMenu - NPC behaviour menu for RDR2 story mode (ScriptHookRDR2).
//
// Every scan interval the mod collects the peds around the player, filters out
// everything that belongs to the story (player, gang, mission entities, optionally
// lawmen) and applies the selected mode to the rest. Everything it changes is
// tracked per ped and undone when the ped leaves, the mode changes or the player
// resets.

#include "script.h"

#include <cstdio>
#include <string>

#include "config.h"
#include "keys.h"
#include "log.h"
#include "modes.h"
#include "natives.h"
#include "ui.h"

static const char* kVersion = "1.0.0";

static HMODULE g_module = nullptr;
static std::string g_dir;
static Config g_cfg;
static NpcManager* g_mgr = nullptr;
static ui::MenuController g_menu;
static ui::Menu g_root, g_settings;

static std::string g_message;
static DWORD g_messageUntil = 0;

void ScriptSetModule(HMODULE module) { g_module = module; }

static std::string moduleDirectory() {
    char path[MAX_PATH] = {};
    GetModuleFileNameA(g_module, path, MAX_PATH);
    std::string dir = path;
    size_t slash = dir.find_last_of("\\/");
    return slash == std::string::npos ? "." : dir.substr(0, slash);
}

// On-screen message only; the manager already logs every state change.
static void notify(const std::string& text) {
    g_message = text;
    g_messageUntil = GetTickCount() + 3500;
}

static void persist() { saveSettings(g_dir, g_cfg.settings); }

static std::string onOff(bool v) { return v ? "[ON]" : "[OFF]"; }

static void buildMenus() {
    Settings& s = g_cfg.settings;
    g_root.title = std::string("SHROUDMENU v") + kVersion;
    g_root.items.clear();
    g_settings.title = "SETTINGS";
    g_settings.items.clear();

    ui::Item mode;
    mode.kind = ui::ItemKind::Choice;
    mode.label = "NPC mode";
    mode.value = [] { return std::string(modeName(g_mgr->mode())); };
    mode.change = [](int dir) {
        int n = static_cast<int>(Mode::Count);
        int next = (static_cast<int>(g_mgr->mode()) + dir + n) % n;
        g_mgr->setMode(static_cast<Mode>(next));
        notify(std::string("NPC mode: ") + modeName(g_mgr->mode()));
    };
    g_root.items.push_back(mode);

    ui::Item glass;
    glass.kind = ui::ItemKind::Toggle;
    glass.label = "Glass jaw (fall on hit)";
    glass.value = [] { return onOff(g_mgr->glassJaw()); };
    glass.change = [](int) {
        g_mgr->setGlassJaw(!g_mgr->glassJaw());
        notify(std::string("Glass jaw ") + (g_mgr->glassJaw() ? "on" : "off"));
    };
    g_root.items.push_back(glass);

    ui::Item settings;
    settings.kind = ui::ItemKind::Submenu;
    settings.label = "Settings";
    settings.submenu = &g_settings;
    g_root.items.push_back(settings);

    ui::Item reset;
    reset.kind = ui::ItemKind::Action;
    reset.label = "Reset all NPCs";
    reset.select = [] { g_mgr->resetAll(); notify("All NPCs restored"); };
    g_root.items.push_back(reset);

    ui::Item test;
    test.kind = ui::ItemKind::Action;
    test.label = "Speech test (nearest NPC)";
    test.select = [] {
        std::string info;
        g_mgr->speechTest(PLAYER_PED_ID(), &info);
        notify(info);
    };
    g_root.items.push_back(test);

    ui::Item info;
    info.kind = ui::ItemKind::Label;
    info.label = "by shrouddeath";
    info.value = [] {
        char buf[64];
        snprintf(buf, sizeof(buf), "%d near / %zu tracked", static_cast<int>(g_mgr->lastScan().inside.size()), g_mgr->trackedCount());
        return std::string(buf);
    };
    g_root.items.push_back(info);

    // ---- settings ----
    auto toggle = [&](const char* label, bool* flag, bool live) {
        ui::Item it;
        it.kind = ui::ItemKind::Toggle;
        it.label = label;
        it.value = [flag] { return onOff(*flag); };
        it.change = [flag, live](int) {
            *flag = !*flag;
            persist();
            if (live) g_mgr->settingsChanged();
        };
        g_settings.items.push_back(it);
    };

    ui::Item radius;
    radius.kind = ui::ItemKind::Number;
    radius.label = "Radius";
    radius.value = [&s] { return std::to_string(s.radius) + " m"; };
    radius.change = [&s](int dir) {
        s.radius += dir * 10;
        if (s.radius < 10) s.radius = 150;
        if (s.radius > 150) s.radius = 10;
        persist();
    };
    g_settings.items.push_back(radius);

    toggle("Include animals", &s.includeAnimals, false);        // Panic and glass jaw only (see ini)
    toggle("Exclude lawmen/guards", &s.excludeLawmen, false);

    ui::Item level;
    level.kind = ui::ItemKind::Choice;
    level.label = "Drunk level";
    level.value = [&s] {
        static const char* names[] = {"Slightly", "Moderately", "Very"};
        return std::string(names[s.drunkLevel < 0 ? 0 : (s.drunkLevel > 2 ? 2 : s.drunkLevel)]);
    };
    level.change = [&s](int dir) {
        s.drunkLevel = (s.drunkLevel + dir + 3) % 3;
        persist();
    };
    g_settings.items.push_back(level);

    toggle("Drunk stumbling", &s.drunkStumble, false);
    toggle("Give weapons (Aggressive)", &s.giveWeapons, false);
    toggle("Disable law (Aggressive)", &s.disableLaw, true);
    toggle("Bump triggers glass jaw", &s.bumpTrigger, false);
    toggle("No retaliation (glass jaw)", &s.noRetaliation, false);
    toggle("Show overlay", &s.showOverlay, false);

    g_menu.setRoot(&g_root);
    ui::Layout layout;
    layout.x = g_cfg.menuX;
    layout.y = g_cfg.menuY;
    layout.width = g_cfg.menuWidth;
    g_menu.setLayout(layout);
}

static bool playerUsable() {
    if (NETWORK_IS_SESSION_STARTED()) return false;   // never touch Red Dead Online
    if (IS_LOADING_SCREEN_VISIBLE() || IS_SCREEN_FADED_OUT()) return false;
    if (!IS_PLAYER_PLAYING(PLAYER_ID())) return false;
    Ped ped = PLAYER_PED_ID();
    return DOES_ENTITY_EXIST(ped) && !IS_ENTITY_DEAD(ped);
}

static void drawOverlay() {
    const ScanResult& scan = g_mgr->lastScan();
    char buf[200];
    if (g_mgr->mode() == Mode::Off && !g_mgr->glassJaw()) {
        snprintf(buf, sizeof(buf), "ShroudMenu: %s opens the menu", keyName(g_cfg.keyMenu).c_str());
    } else {
        snprintf(buf, sizeof(buf), "ShroudMenu | mode: %s | glass jaw: %s | NPCs: %d near, %zu tracked (skipped: %d story, %d law, %d friends)",
                 modeName(g_mgr->mode()), g_mgr->glassJaw() ? "on" : "off", static_cast<int>(scan.inside.size()),
                 g_mgr->trackedCount(), scan.skippedMission, scan.skippedLaw, scan.skippedFriend);
    }
    ui::text(buf, 0.02f, 0.945f, {210, 210, 210, 210}, g_cfg.textScale * 0.85f);
}

void ScriptMain() {
    g_dir = moduleDirectory();
    logfile::open(g_dir + "\\ShroudMenu.log");
    g_cfg = loadConfig(g_dir);
    logfile::setLevel(g_cfg.logLevel);
    g_cfg.settings = loadSettings(g_dir, g_cfg.settings);

    logfile::write("ShroudMenu %s by shrouddeath loaded. menu key %s, radius %d m, scan every %d ms, text backend %s",
                   kVersion, keyName(g_cfg.keyMenu).c_str(), g_cfg.settings.radius, g_cfg.scanIntervalMs,
                   g_cfg.textBackendBg ? "bg" : "legacy");
    if (!g_cfg.enabled) {
        logfile::write("Disabled in ShroudMenu.ini.");
        return;
    }

    ui::init(g_cfg.textBackendBg, g_cfg.textScale);
    for (int vk : {g_cfg.keyMenu, g_cfg.keyUp, g_cfg.keyDown, g_cfg.keyLeft, g_cfg.keyRight, g_cfg.keySelect, g_cfg.keyBack,
                   0x26, 0x28, 0x25, 0x27, 0x0D, 0x08})
        keys::watch(vk);

    NpcManager manager(g_cfg, g_cfg.settings);
    g_mgr = &manager;
    buildMenus();

    DWORD nextTick = 0;
    bool wasUsable = false;
    for (;;) {
        keys::update();
        bool usable = playerUsable();
        if (!usable) {
            if (wasUsable) {
                // loading screen, death or online: drop everything we hold
                if (manager.trackedCount() > 0) manager.resetAll();
                g_menu.close();
            }
            wasUsable = false;
            WAIT(100);
            continue;
        }
        wasUsable = true;

        if (keys::pressed(g_cfg.keyMenu)) g_menu.toggle();
        if (g_menu.isOpen())
            g_menu.input(g_cfg.keyUp, g_cfg.keyDown, g_cfg.keyLeft, g_cfg.keyRight, g_cfg.keySelect, g_cfg.keyBack);

        DWORD now = GetTickCount();
        bool paused = IS_PAUSE_MENU_ACTIVE();
        if (!paused && static_cast<LONG>(now - nextTick) >= 0) {
            // Cutscenes and scripted moments switch player control off: leave the NPCs alone then.
            if (IS_PLAYER_CONTROL_ON(PLAYER_ID())) manager.tick(PLAYER_PED_ID());
            nextTick = now + static_cast<DWORD>(g_cfg.scanIntervalMs);
        }

        if (!paused) {
            if (g_menu.isOpen()) g_menu.draw();
            if (g_cfg.settings.showOverlay) drawOverlay();
            if (!g_message.empty() && now <= g_messageUntil) ui::text(g_message, 0.30f, 0.04f, {255, 240, 200, 240});
        }
        WAIT(0);
    }
}
