#pragma once
#include <memory>
#include <random>
#include <string>
#include <unordered_map>
#include <unordered_set>

#include "config.h"
#include "natives.h"
#include "scanner.h"

// Everything we changed on one NPC, so it can be undone when the NPC leaves,
// the mode changes, or the mod is reset.
struct TrackedNpc {
    bool entered = false;
    bool human = true;
    Hash origGroup = 0;
    Hash weaponGiven = 0;
    bool eventsBlocked = false;
    bool glassEntered = false;
    unsigned long nextSpeech = 0;
    unsigned long nextStumble = 0;
    unsigned long nextRetask = 0;
    unsigned long ragdollUntil = 0;
    unsigned long lastSeen = 0;
    int hits = 0;
};

struct ModeContext {
    const Config* cfg = nullptr;
    const Settings* settings = nullptr;
    Ped player = 0;
    unsigned long now = 0;
    float radius = 60.0f;
    float exitRadius = 120.0f;
    std::mt19937* rng = nullptr;

    int randRange(int lo, int hi) const;
    unsigned long randMs(int minSec, int maxSec) const;
};

struct NpcMode {
    virtual ~NpcMode() = default;
    virtual Mode id() const = 0;
    virtual void onModeStart(ModeContext&) {}
    virtual void onModeStop(ModeContext&) {}
    virtual void onEnter(Ped ped, TrackedNpc& s, ModeContext& ctx) = 0;   // once, when the NPC is first affected
    virtual void onTick(Ped ped, TrackedNpc& s, ModeContext& ctx, float dist) = 0;   // every scan while tracked
    virtual void onExit(Ped ped, TrackedNpc& s, ModeContext& ctx) = 0;    // undo; ped is verified to exist
};

class NpcManager {
public:
    NpcManager(const Config& cfg, Settings& settings);

    Mode mode() const { return m_mode; }
    void setMode(Mode m);
    bool glassJaw() const { return m_glassJaw; }
    void setGlassJaw(bool on);
    void settingsChanged();

    void tick(Ped player);     // scan and apply, call every scan interval
    void resetAll();           // undo everything, keep the mode
    size_t trackedCount() const { return m_tracked.size(); }
    const ScanResult& lastScan() const { return m_scan; }
    bool speechTest(Ped player, std::string* info);

private:
    ModeContext makeContext(Ped player);
    NpcMode& current();
    void exitNpc(Ped ped, TrackedNpc& s, ModeContext& ctx);
    void exitAll(ModeContext& ctx);
    void glassJawTick(Ped ped, TrackedNpc& s, ModeContext& ctx, float dist);
    void glassJawExit(Ped ped, TrackedNpc& s);

    const Config& m_cfg;
    Settings& m_settings;
    Mode m_mode = Mode::Off;
    bool m_glassJaw = false;
    Ped m_lastPlayer = 0;
    std::unordered_map<Ped, TrackedNpc> m_tracked;
    std::unique_ptr<NpcMode> m_modes[static_cast<int>(Mode::Count)];
    std::mt19937 m_rng;
    ScanResult m_scan;
};
