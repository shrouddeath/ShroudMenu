#include "config.h"

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <map>
#include <sstream>

Hash joaat(const std::string& s) { return joaat(s.c_str()); }

std::string trim(const std::string& s) {
    size_t a = 0, b = s.size();
    while (a < b && isspace(static_cast<unsigned char>(s[a]))) ++a;
    while (b > a && isspace(static_cast<unsigned char>(s[b - 1]))) --b;
    return s.substr(a, b - a);
}

std::string lower(const std::string& s) {
    std::string out = s;
    std::transform(out.begin(), out.end(), out.begin(), [](unsigned char c) { return static_cast<char>(tolower(c)); });
    return out;
}

std::vector<std::string> splitList(const std::string& text) {
    std::vector<std::string> out;
    std::stringstream ss(text);
    std::string item;
    while (std::getline(ss, item, ',')) {
        item = trim(item);
        if (!item.empty()) out.push_back(item);
    }
    return out;
}

static const std::map<std::string, int>& namedKeys() {
    static const std::map<std::string, int> named = {
        {"up", 0x26}, {"down", 0x28}, {"left", 0x25}, {"right", 0x27},
        {"enter", 0x0D}, {"return", 0x0D}, {"backspace", 0x08}, {"space", 0x20},
        {"tab", 0x09}, {"escape", 0x1B}, {"esc", 0x1B},
        {"home", 0x24}, {"end", 0x23}, {"pageup", 0x21}, {"pagedown", 0x22}, {"insert", 0x2D}, {"delete", 0x2E},
        {"numpad0", 0x60}, {"numpad1", 0x61}, {"numpad2", 0x62}, {"numpad3", 0x63}, {"numpad4", 0x64},
        {"numpad5", 0x65}, {"numpad6", 0x66}, {"numpad7", 0x67}, {"numpad8", 0x68}, {"numpad9", 0x69},
        {"numpadplus", 0x6B}, {"numpadminus", 0x6D}, {"numpadmultiply", 0x6A}, {"numpaddivide", 0x6F},
        {"numpaddecimal", 0x6E},
    };
    return named;
}

int parseKey(const std::string& textIn) {
    std::string text = lower(trim(textIn));
    if (text.empty() || text == "none" || text == "off") return 0;
    if (text.rfind("0x", 0) == 0) return static_cast<int>(strtol(text.c_str(), nullptr, 16));
    auto it = namedKeys().find(text);
    if (it != namedKeys().end()) return it->second;
    if (text.size() >= 2 && text[0] == 'f') {
        int n = atoi(text.c_str() + 1);
        if (n >= 1 && n <= 24) return 0x70 + n - 1;
    }
    if (text.size() == 1) {
        char c = text[0];
        if (c >= 'a' && c <= 'z') return toupper(c);
        if (c >= '0' && c <= '9') return c;
    }
    return 0;
}

std::string keyName(int vk) {
    if (vk >= 0x70 && vk <= 0x87) return "F" + std::to_string(vk - 0x70 + 1);
    for (const auto& [name, code] : namedKeys())
        if (code == vk) {
            std::string n = name;
            n[0] = static_cast<char>(toupper(n[0]));
            return n;
        }
    if ((vk >= 'A' && vk <= 'Z') || (vk >= '0' && vk <= '9')) return std::string(1, static_cast<char>(vk));
    char buf[16];
    snprintf(buf, sizeof(buf), "0x%02X", vk);
    return buf;
}

Hash parseSpeechParams(const std::string& textIn) {
    std::string text = lower(trim(textIn));
    if (text.empty()) return joaat("SPEECH_PARAMS_FORCE");
    if (text.rfind("0x", 0) == 0) return static_cast<Hash>(strtoul(text.c_str(), nullptr, 16));
    if (text == "lms") return 0xE7176FB2;   // hash that worked in the rdr2mods community test (name unknown)
    if (text.rfind("speech_params_", 0) == 0) return joaat(text);
    return joaat("speech_params_" + text);
}

const char* modeName(Mode m) {
    switch (m) {
    case Mode::Off: return "Off";
    case Mode::Drunk: return "Drunk";
    case Mode::Insult: return "Insulting";
    case Mode::Aggressive: return "Aggressive";
    case Mode::Panic: return "Panic";
    case Mode::Robbery: return "Robbery";
    case Mode::Lasso: return "Lasso";
    default: return "?";
    }
}

// ---- ini ---------------------------------------------------------------------

using Ini = std::map<std::string, std::map<std::string, std::string>>;   // section -> key -> value

static Ini readIni(const std::string& path) {
    Ini ini;
    std::ifstream in(path);
    std::string line, section;
    while (std::getline(in, line)) {
        line = trim(line);
        if (line.empty() || line[0] == ';' || line[0] == '#') continue;
        if (line.front() == '[' && line.back() == ']') {
            section = lower(trim(line.substr(1, line.size() - 2)));
            continue;
        }
        size_t eq = line.find('=');
        if (eq == std::string::npos) continue;
        std::string key = lower(trim(line.substr(0, eq)));
        std::string value = trim(line.substr(eq + 1));
        size_t comment = value.find(" ;");
        if (comment != std::string::npos) value = trim(value.substr(0, comment));
        ini[section][key] = value;
    }
    return ini;
}

struct Reader {
    const Ini& ini;
    bool has(const char* sec, const char* key) const {
        auto s = ini.find(lower(sec));
        return s != ini.end() && s->second.count(lower(key)) > 0;
    }
    std::string str(const char* sec, const char* key, const std::string& def) const {
        auto s = ini.find(lower(sec));
        if (s == ini.end()) return def;
        auto k = s->second.find(lower(key));
        return k == s->second.end() ? def : k->second;
    }
    int integer(const char* sec, const char* key, int def) const {
        std::string v = str(sec, key, "");
        return v.empty() ? def : atoi(v.c_str());
    }
    float real(const char* sec, const char* key, float def) const {
        std::string v = str(sec, key, "");
        return v.empty() ? def : static_cast<float>(atof(v.c_str()));
    }
    bool boolean(const char* sec, const char* key, bool def) const {
        std::string v = lower(str(sec, key, ""));
        if (v.empty()) return def;
        return v == "1" || v == "true" || v == "on" || v == "yes";
    }
    int key(const char* sec, const char* k, int def) const {
        return has(sec, k) ? parseKey(str(sec, k, "")) : def;
    }
    std::vector<std::string> list(const char* sec, const char* key, const char* def) const {
        return splitList(str(sec, key, def));
    }
};

Config loadConfig(const std::string& dir) {
    Config c;
    Ini ini = readIni(dir + "\\ShroudMenu.ini");
    Reader r{ini};

    c.enabled = r.boolean("General", "Enabled", c.enabled);
    c.scanIntervalMs = std::max(100, r.integer("General", "ScanIntervalMs", c.scanIntervalMs));
    c.logLevel = r.integer("General", "LogLevel", c.logLevel);

    Settings& s = c.settings;
    s.radius = std::clamp(r.integer("General", "Radius", s.radius), 10, 150);
    s.showOverlay = r.boolean("General", "ShowOverlay", s.showOverlay);
    s.includeAnimals = r.boolean("Filter", "IncludeAnimals", s.includeAnimals);
    s.excludeLawmen = r.boolean("Filter", "ExcludeLawmen", s.excludeLawmen);

    c.keyMenu = r.key("Keys", "Menu", c.keyMenu);
    c.keyUp = r.key("Keys", "Up", c.keyUp);
    c.keyDown = r.key("Keys", "Down", c.keyDown);
    c.keyLeft = r.key("Keys", "Left", c.keyLeft);
    c.keyRight = r.key("Keys", "Right", c.keyRight);
    c.keySelect = r.key("Keys", "Select", c.keySelect);
    c.keyBack = r.key("Keys", "Back", c.keyBack);

    s.drunkLevel = std::clamp(r.integer("Drunk", "Level", s.drunkLevel), 0, 2);
    s.drunkStumble = r.boolean("Drunk", "Stumble", s.drunkStumble);
    c.drunkStumbleMinSec = r.integer("Drunk", "StumbleMinSec", c.drunkStumbleMinSec);
    c.drunkStumbleMaxSec = r.integer("Drunk", "StumbleMaxSec", c.drunkStumbleMaxSec);
    c.drunkStumbleRagdollType = r.integer("Drunk", "StumbleRagdollType", c.drunkStumbleRagdollType);
    c.drunkStumbleMinMs = r.integer("Drunk", "StumbleMinMs", c.drunkStumbleMinMs);
    c.drunkStumbleMaxMs = r.integer("Drunk", "StumbleMaxMs", c.drunkStumbleMaxMs);
    c.drunkSpeech = r.boolean("Drunk", "Speech", c.drunkSpeech);
    c.drunkSpeechMinSec = r.integer("Drunk", "SpeechMinSec", c.drunkSpeechMinSec);
    c.drunkSpeechMaxSec = r.integer("Drunk", "SpeechMaxSec", c.drunkSpeechMaxSec);
    c.drunkSpeechParams = parseSpeechParams(r.str("Drunk", "SpeechParams", "force_shouted_critical"));
    c.drunkMotivation = r.boolean("Drunk", "UseMotivation", c.drunkMotivation);
    c.drunkLines = r.list("Drunk", "Lines", "DRUNK,DRUNK,DRUNK,GENERIC_CURSE_MED,CHALLENGE_THREATEN_DRUNK");

    c.insultDistance = r.real("Insult", "DistanceM", c.insultDistance);
    c.insultMinSec = r.integer("Insult", "MinSec", c.insultMinSec);
    c.insultMaxSec = r.integer("Insult", "MaxSec", c.insultMaxSec);
    c.insultFaceTarget = r.boolean("Insult", "FaceTarget", c.insultFaceTarget);
    c.insultNeedLos = r.boolean("Insult", "NeedLineOfSight", c.insultNeedLos);
    c.insultSpeechParams = parseSpeechParams(r.str("Insult", "SpeechParams", "force"));
    c.insultLines = r.list("Insult", "Lines",
                           "GENERIC_INSULT_HIGH,GENERIC_INSULT_HIGH,GENERIC_INSULT_MED,GENERIC_CURSE_HIGH,GENERIC_CURSE_MED,"
                           "GET_LOST,WHATS_YOUR_PROBLEM,CHALLENGE_THREATEN,CIVILIAN_TAUNT_MED,GENERIC_ANGRY_SHOUT,DISMISSIVE_REACT_ANNOYED");

    s.giveWeapons = r.boolean("Aggressive", "GiveWeapons", s.giveWeapons);
    s.disableLaw = r.boolean("Aggressive", "DisableLaw", s.disableLaw);
    c.weapons = r.list("Aggressive", "Weapons", "WEAPON_MELEE_KNIFE,WEAPON_REVOLVER_CATTLEMAN");
    c.accuracy = std::clamp(r.integer("Aggressive", "Accuracy", c.accuracy), 0, 100);
    c.combatRetaskSec = std::max(1, r.integer("Aggressive", "RetaskSec", c.combatRetaskSec));

    c.ragdollMinMs = r.integer("GlassJaw", "RagdollMinMs", c.ragdollMinMs);
    c.ragdollMaxMs = r.integer("GlassJaw", "RagdollMaxMs", c.ragdollMaxMs);
    c.ragdollType = r.integer("GlassJaw", "RagdollType", c.ragdollType);
    c.pushForce = r.real("GlassJaw", "PushForce", c.pushForce);
    c.healAfterHit = r.boolean("GlassJaw", "HealAfterHit", c.healAfterHit);
    s.bumpTrigger = r.boolean("GlassJaw", "BumpTrigger", s.bumpTrigger);
    s.noRetaliation = r.boolean("GlassJaw", "NoRetaliation", s.noRetaliation);

    c.panicSpeech = r.boolean("Panic", "Speech", c.panicSpeech);
    c.panicMinSec = r.integer("Panic", "MinSec", c.panicMinSec);
    c.panicMaxSec = r.integer("Panic", "MaxSec", c.panicMaxSec);
    c.panicSpeechParams = parseSpeechParams(r.str("Panic", "SpeechParams", "force_shouted_critical"));
    c.panicSpeed = r.real("Panic", "Speed", c.panicSpeed);
    c.panicFleeType = r.integer("Panic", "FleeType", c.panicFleeType);
    c.panicLines = r.list("Panic", "Lines",
                          "PANIC_HELP,SCARED_HELP,GENERIC_FRIGHTENED_HIGH,GENERIC_SHOCKED_HIGH,PLEAD,SCREAM_TERROR,SCREAM_SHOCKED,GUN_RUN");

    c.robDistance = r.real("Robbery", "DistanceM", c.robDistance);
    c.robHoldupSec = std::max(2, r.integer("Robbery", "HoldupSec", c.robHoldupSec));
    c.robCooldownMinSec = r.integer("Robbery", "CooldownMinSec", c.robCooldownMinSec);
    c.robCooldownMaxSec = r.integer("Robbery", "CooldownMaxSec", c.robCooldownMaxSec);
    c.robWeapon = r.str("Robbery", "Weapon", c.robWeapon);
    c.robSpeechParams = parseSpeechParams(r.str("Robbery", "SpeechParams", "force"));
    c.robRobberLines = r.list("Robbery", "RobberLines", "CHALLENGE_THREATEN_ARMED,CHALLENGE_THREATEN,GENERIC_CURSE_HIGH");
    c.robVictimLines = r.list("Robbery", "VictimLines", "HANDS_UP,HOLDUP_DONT_SHOOT,HOLDUP_DONT_KILL_ME,PLEAD,INTIMIDATED_ROB");
    c.robHandOverLines = r.list("Robbery", "HandOverLines", "HAND_OVER_MONEY,HOLDUP_GIVE_CASH,PLEAD");
    c.robFleeLines = r.list("Robbery", "FleeLines", "HOLDUP_RUN,HOLDUP_HELP,PANIC_HELP");

    c.lassoDistance = r.real("Lasso", "DistanceM", c.lassoDistance);
    c.lassoThrowDistance = r.real("Lasso", "ThrowDistanceM", c.lassoThrowDistance);
    c.lassoHogtie = r.boolean("Lasso", "Hogtie", c.lassoHogtie);
    c.lassoHogtieHoldSec = r.integer("Lasso", "HogtieHoldSec", c.lassoHogtieHoldSec);
    c.lassoTimeoutSec = std::max(3, r.integer("Lasso", "TimeoutSec", c.lassoTimeoutSec));
    c.lassoCooldownMinSec = r.integer("Lasso", "CooldownMinSec", c.lassoCooldownMinSec);
    c.lassoCooldownMaxSec = r.integer("Lasso", "CooldownMaxSec", c.lassoCooldownMaxSec);
    c.lassoTargetFlees = r.boolean("Lasso", "TargetFlees", c.lassoTargetFlees);
    c.lassoFleeSpeed = r.real("Lasso", "FleeSpeed", c.lassoFleeSpeed);
    c.lassoSpeechParams = parseSpeechParams(r.str("Lasso", "SpeechParams", "force_shouted_critical"));
    c.lassoLassoerLines = r.list("Lasso", "LassoerLines", "HOGTIES_MALE,HOGTIES_FEMALE,CHALLENGE_THREATEN");
    c.lassoTargetLines = r.list("Lasso", "TargetLines", "PLEAD,GENERIC_FRIGHTENED_HIGH,SCARED_HELP");

    c.textBackendBg = lower(r.str("UI", "TextBackend", "bg")) != "legacy";
    c.menuX = r.real("UI", "X", c.menuX);
    c.menuY = r.real("UI", "Y", c.menuY);
    c.menuWidth = r.real("UI", "Width", c.menuWidth);
    c.textScale = r.real("UI", "TextScale", c.textScale);
    return c;
}

// ---- state (menu changes) ----------------------------------------------------

Settings loadSettings(const std::string& dir, const Settings& defaults) {
    Settings s = defaults;
    Ini ini = readIni(dir + "\\ShroudMenu.state");
    if (ini.empty()) return s;
    Reader r{ini};
    s.radius = std::clamp(r.integer("State", "Radius", s.radius), 10, 150);
    s.includeAnimals = r.boolean("State", "IncludeAnimals", s.includeAnimals);
    s.excludeLawmen = r.boolean("State", "ExcludeLawmen", s.excludeLawmen);
    s.drunkLevel = std::clamp(r.integer("State", "DrunkLevel", s.drunkLevel), 0, 2);
    s.drunkStumble = r.boolean("State", "DrunkStumble", s.drunkStumble);
    s.giveWeapons = r.boolean("State", "GiveWeapons", s.giveWeapons);
    s.disableLaw = r.boolean("State", "DisableLaw", s.disableLaw);
    s.bumpTrigger = r.boolean("State", "BumpTrigger", s.bumpTrigger);
    s.noRetaliation = r.boolean("State", "NoRetaliation", s.noRetaliation);
    s.showOverlay = r.boolean("State", "ShowOverlay", s.showOverlay);
    return s;
}

void saveSettings(const std::string& dir, const Settings& s) {
    FILE* f = fopen((dir + "\\ShroudMenu.state").c_str(), "w");
    if (!f) return;
    fprintf(f, "; written by ShroudMenu whenever a setting changes in the menu\n[State]\n");
    fprintf(f, "Radius=%d\nIncludeAnimals=%d\nExcludeLawmen=%d\nDrunkLevel=%d\nDrunkStumble=%d\nGiveWeapons=%d\n"
               "DisableLaw=%d\nBumpTrigger=%d\nNoRetaliation=%d\nShowOverlay=%d\n",
            s.radius, s.includeAnimals ? 1 : 0, s.excludeLawmen ? 1 : 0, s.drunkLevel, s.drunkStumble ? 1 : 0,
            s.giveWeapons ? 1 : 0, s.disableLaw ? 1 : 0, s.bumpTrigger ? 1 : 0, s.noRetaliation ? 1 : 0,
            s.showOverlay ? 1 : 0);
    fclose(f);
}
