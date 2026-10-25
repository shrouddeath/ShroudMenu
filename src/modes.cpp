#include "modes.h"

#include <algorithm>
#include <cmath>

#include "log.h"
#include "speech.h"

// ---- helpers -------------------------------------------------------------------

int ModeContext::randRange(int lo, int hi) const {
    if (hi < lo) std::swap(lo, hi);
    std::uniform_int_distribution<int> d(lo, hi);
    return d(*rng);
}

unsigned long ModeContext::randMs(int minSec, int maxSec) const {
    return static_cast<unsigned long>(randRange(minSec * 1000, maxSec * 1000));
}

static float distanceTo(Ped a, Ped b) { return distanceBetween(GET_ENTITY_COORDS(a), GET_ENTITY_COORDS(b)); }

namespace {

// Relationship values (RDR3): 1 respect, 3 ignore, 6 hate
const int kRelRespect = 1, kRelIgnore = 3, kRelHate = 6;
// eCombatAttribute
const int kCaAlwaysFight = 5, kCaAlwaysFlee = 17, kCaFightArmedWhenUnarmed = 46, kCaDisableFleeFromCombat = 58;
// eMotivationState
const int kMotivationDrunk = 10;
const Hash kAddReasonDefault = joaat("ADD_REASON_DEFAULT");

// ---- Off -------------------------------------------------------------------------

struct OffMode : NpcMode {
    Mode id() const override { return Mode::Off; }
    void onEnter(Ped, TrackedNpc&, ModeContext&) override {}
    void onTick(Ped, TrackedNpc&, ModeContext&, float) override {}
    void onExit(Ped, TrackedNpc&, ModeContext&) override {}
};

// ---- Drunk -----------------------------------------------------------------------

struct DrunkMode : NpcMode {
    Mode id() const override { return Mode::Drunk; }

    float level(const ModeContext& ctx) const {
        int i = std::clamp(ctx.settings->drunkLevel, 0, 2);
        return ctx.cfg->drunkLevels[i];
    }

    void onEnter(Ped ped, TrackedNpc& s, ModeContext& ctx) override {
        _SET_PED_DRUNKNESS(ped, true, level(ctx));
        SET_PED_IS_DRUNK(ped, true);
        if (ctx.cfg->drunkMotivation) _SET_PED_MOTIVATION(ped, kMotivationDrunk, 1.0f, 0);
        s.nextStumble = ctx.now + ctx.randMs(2, ctx.cfg->drunkStumbleMaxSec);
        s.nextSpeech = ctx.now + ctx.randMs(2, ctx.cfg->drunkSpeechMaxSec);
        s.nextRetask = ctx.now + 10000;
        logfile::debug("drunk: enter ped %d level %.2f", ped, level(ctx));
    }

    void onTick(Ped ped, TrackedNpc& s, ModeContext& ctx, float dist) override {
        const Config& c = *ctx.cfg;
        if (ctx.now >= s.nextRetask) {   // the game may sober them up again
            _SET_PED_DRUNKNESS(ped, true, level(ctx));
            s.nextRetask = ctx.now + 10000;
        }
        if (ctx.settings->drunkStumble && s.human && ctx.now >= s.nextStumble && dist < 50.0f && !IS_PED_RAGDOLL(ped) &&
            !IS_PED_ON_MOUNT(ped) && !IS_PED_IN_MELEE_COMBAT(ped)) {
            SET_PED_CAN_RAGDOLL(ped, true);
            SET_PED_TO_RAGDOLL(ped, c.drunkStumbleMinMs, c.drunkStumbleMaxMs, c.drunkStumbleRagdollType);
            s.nextStumble = ctx.now + ctx.randMs(c.drunkStumbleMinSec, c.drunkStumbleMaxSec);
        }
        if (c.drunkSpeech && ctx.now >= s.nextSpeech && dist < 25.0f && !IS_ANY_SPEECH_PLAYING(ped) &&
            speech::globalSlot(ctx.now, 1200)) {
            speech::sayOneOf(ped, c.drunkLines, c.drunkSpeechParams, ctx.player);
            s.nextSpeech = ctx.now + ctx.randMs(c.drunkSpeechMinSec, c.drunkSpeechMaxSec);
        }
    }

    void onExit(Ped ped, TrackedNpc&, ModeContext&) override {
        _SET_PED_DRUNKNESS(ped, false, 0.0f);
        SET_PED_IS_DRUNK(ped, false);
    }
};

// ---- Insulting -------------------------------------------------------------------

struct InsultMode : NpcMode {
    Mode id() const override { return Mode::Insult; }

    void onEnter(Ped, TrackedNpc& s, ModeContext& ctx) override {
        s.nextSpeech = ctx.now + ctx.randMs(1, 6);
    }

    void onTick(Ped ped, TrackedNpc& s, ModeContext& ctx, float dist) override {
        const Config& c = *ctx.cfg;
        if (dist > c.insultDistance || ctx.now < s.nextSpeech) return;
        if (IS_PED_IN_COMBAT(ped, ctx.player) || IS_PED_FLEEING(ped) || IS_PED_RAGDOLL(ped)) return;
        if (c.insultNeedLos && !HAS_ENTITY_CLEAR_LOS_TO_ENTITY(ped, ctx.player)) return;
        if (IS_ANY_SPEECH_PLAYING(ped) || !speech::globalSlot(ctx.now, 1500)) return;

        if (c.insultFaceTarget) {
            TASK_LOOK_AT_ENTITY(ped, ctx.player, 4000);
            if (IS_PED_STOPPED(ped) && !IS_PED_USING_ANY_SCENARIO(ped)) TASK_TURN_PED_TO_FACE_ENTITY(ped, ctx.player, 2500);
        }
        std::string used;
        speech::sayOneOf(ped, c.insultLines, c.insultSpeechParams, ctx.player, &used);
        s.nextSpeech = ctx.now + ctx.randMs(c.insultMinSec, c.insultMaxSec);
    }

    void onExit(Ped ped, TrackedNpc&, ModeContext&) override { TASK_CLEAR_LOOK_AT(ped); }
};

// ---- Aggressive ------------------------------------------------------------------

struct AggressiveMode : NpcMode {
    Hash hostile = 0;
    Hash playerGroup = 0;
    bool lawDisabled = false;

    Mode id() const override { return Mode::Aggressive; }

    void onModeStart(ModeContext& ctx) override {
        if (hostile == 0) {
            if (!ADD_RELATIONSHIP_GROUP("SHROUD_HOSTILE", &hostile) || hostile == 0) hostile = joaat("SHROUD_HOSTILE");
            logfile::write("aggressive: relationship group SHROUD_HOSTILE = 0x%08X", hostile);
        }
        playerGroup = GET_PED_RELATIONSHIP_GROUP_HASH(ctx.player);
        SET_RELATIONSHIP_BETWEEN_GROUPS(kRelHate, hostile, playerGroup);
        SET_RELATIONSHIP_BETWEEN_GROUPS(kRelHate, playerGroup, hostile);
        SET_RELATIONSHIP_BETWEEN_GROUPS(kRelRespect, hostile, hostile);
        applyLaw(*ctx.settings);
    }

    void onModeStop(ModeContext&) override {
        if (hostile && playerGroup) {
            SET_RELATIONSHIP_BETWEEN_GROUPS(kRelIgnore, hostile, playerGroup);
            SET_RELATIONSHIP_BETWEEN_GROUPS(kRelIgnore, playerGroup, hostile);
        }
        if (lawDisabled) { _SET_LAW_DISABLED(false); lawDisabled = false; }
    }

    void applyLaw(const Settings& s) {
        if (s.disableLaw != lawDisabled) {
            _SET_LAW_DISABLED(s.disableLaw);
            lawDisabled = s.disableLaw;
            logfile::write("aggressive: law response %s", lawDisabled ? "disabled" : "enabled");
        }
    }

    void onEnter(Ped ped, TrackedNpc& s, ModeContext& ctx) override {
        const Config& c = *ctx.cfg;
        s.origGroup = GET_PED_RELATIONSHIP_GROUP_HASH(ped);
        SET_PED_RELATIONSHIP_GROUP_HASH(ped, hostile);
        SET_PED_COMBAT_ATTRIBUTES(ped, kCaAlwaysFight, true);
        SET_PED_COMBAT_ATTRIBUTES(ped, kCaFightArmedWhenUnarmed, true);
        SET_PED_COMBAT_ATTRIBUTES(ped, kCaDisableFleeFromCombat, true);
        SET_PED_COMBAT_ATTRIBUTES(ped, kCaAlwaysFlee, false);
        SET_PED_COMBAT_MOVEMENT(ped, 2);
        SET_PED_COMBAT_ABILITY(ped, 1);
        SET_PED_COMBAT_RANGE(ped, 0);
        SET_PED_ACCURACY(ped, c.accuracy);
        SET_PED_SEEING_RANGE(ped, 120.0f);
        SET_PED_HEARING_RANGE(ped, 120.0f);
        if (ctx.settings->giveWeapons && s.human && !c.weapons.empty()) {
            const std::string& name = c.weapons[static_cast<size_t>(ctx.randRange(0, static_cast<int>(c.weapons.size()) - 1))];
            Hash weapon = joaat(name);
            GIVE_WEAPON_TO_PED(ped, weapon, 60, true, kAddReasonDefault);
            SET_PED_INFINITE_AMMO(ped, true, weapon);
            s.weaponGiven = weapon;
            logfile::debug("aggressive: ped %d armed with %s", ped, name.c_str());
        }
        TASK_COMBAT_PED(ped, ctx.player);
        SET_PED_KEEP_TASK(ped, true);
        s.nextRetask = ctx.now + static_cast<unsigned long>(c.combatRetaskSec) * 1000;
    }

    void onTick(Ped ped, TrackedNpc& s, ModeContext& ctx, float) override {
        if (ctx.now < s.nextRetask) return;
        if (!IS_PED_IN_COMBAT(ped, ctx.player) && !IS_PED_RAGDOLL(ped)) TASK_COMBAT_PED(ped, ctx.player);
        s.nextRetask = ctx.now + static_cast<unsigned long>(ctx.cfg->combatRetaskSec) * 1000;
    }

    void onExit(Ped ped, TrackedNpc& s, ModeContext&) override {
        if (s.origGroup) SET_PED_RELATIONSHIP_GROUP_HASH(ped, s.origGroup);
        SET_PED_COMBAT_ATTRIBUTES(ped, kCaAlwaysFight, false);
        SET_PED_COMBAT_ATTRIBUTES(ped, kCaFightArmedWhenUnarmed, false);
        SET_PED_COMBAT_ATTRIBUTES(ped, kCaDisableFleeFromCombat, false);
        SET_PED_KEEP_TASK(ped, false);
        if (s.weaponGiven) {
            SET_PED_INFINITE_AMMO(ped, false, s.weaponGiven);
            REMOVE_WEAPON_FROM_PED(ped, s.weaponGiven);
            s.weaponGiven = 0;
        }
        if (!IS_ENTITY_DEAD(ped)) CLEAR_PED_TASKS(ped);
    }
};

// ---- Panic -----------------------------------------------------------------------

struct PanicMode : NpcMode {
    Mode id() const override { return Mode::Panic; }

    float fleeDistance(const ModeContext& ctx) const { return std::max(40.0f, ctx.exitRadius - 10.0f); }

    void flee(Ped ped, ModeContext& ctx) {
        TASK_SMART_FLEE_PED(ped, ctx.player, fleeDistance(ctx), -1, ctx.cfg->panicFleeType, ctx.cfg->panicSpeed);
        SET_PED_KEEP_TASK(ped, true);
    }

    void onEnter(Ped ped, TrackedNpc& s, ModeContext& ctx) override {
        flee(ped, ctx);
        s.nextSpeech = ctx.now + static_cast<unsigned long>(ctx.randRange(200, 2500));
        s.nextRetask = ctx.now + 3000;
    }

    void onTick(Ped ped, TrackedNpc& s, ModeContext& ctx, float dist) override {
        const Config& c = *ctx.cfg;
        if (ctx.now >= s.nextRetask) {
            if (!IS_PED_FLEEING(ped) && !IS_PED_RAGDOLL(ped) && dist < fleeDistance(ctx) - 10.0f) flee(ped, ctx);
            s.nextRetask = ctx.now + 3000;
        }
        if (c.panicSpeech && s.human && ctx.now >= s.nextSpeech && dist < 45.0f && !IS_ANY_SPEECH_PLAYING(ped) &&
            speech::globalSlot(ctx.now, 700)) {
            speech::sayOneOf(ped, c.panicLines, c.panicSpeechParams, ctx.player);
            s.nextSpeech = ctx.now + ctx.randMs(c.panicMinSec, c.panicMaxSec);
        }
    }

    void onExit(Ped ped, TrackedNpc&, ModeContext&) override {
        SET_PED_KEEP_TASK(ped, false);
        if (!IS_ENTITY_DEAD(ped)) CLEAR_PED_TASKS(ped);
    }
};

} // namespace

// ---- manager -----------------------------------------------------------------------

NpcManager::NpcManager(const Config& cfg, Settings& settings)
    : m_cfg(cfg), m_settings(settings), m_rng(static_cast<unsigned>(GetTickCount())) {
    srand(static_cast<unsigned>(GetTickCount()));   // speech::sayOneOf picks lines with rand()
    m_modes[static_cast<int>(Mode::Off)] = std::make_unique<OffMode>();
    m_modes[static_cast<int>(Mode::Drunk)] = std::make_unique<DrunkMode>();
    m_modes[static_cast<int>(Mode::Insult)] = std::make_unique<InsultMode>();
    m_modes[static_cast<int>(Mode::Aggressive)] = std::make_unique<AggressiveMode>();
    m_modes[static_cast<int>(Mode::Panic)] = std::make_unique<PanicMode>();
}

NpcMode& NpcManager::current() { return *m_modes[static_cast<int>(m_mode)]; }

ModeContext NpcManager::makeContext(Ped player) {
    ModeContext ctx;
    ctx.cfg = &m_cfg;
    ctx.settings = &m_settings;
    ctx.player = player;
    ctx.now = GetTickCount();
    ctx.radius = static_cast<float>(m_settings.radius);
    ctx.exitRadius = ctx.radius * 2.0f;
    ctx.rng = &m_rng;
    return ctx;
}

void NpcManager::setMode(Mode m) {
    if (m == m_mode) return;
    ModeContext ctx = makeContext(m_lastPlayer ? m_lastPlayer : PLAYER_PED_ID());
    exitAll(ctx);
    current().onModeStop(ctx);
    logfile::write("mode: %s -> %s", modeName(m_mode), modeName(m));
    m_mode = m;
    current().onModeStart(ctx);
}

void NpcManager::setGlassJaw(bool on) {
    if (on == m_glassJaw) return;
    m_glassJaw = on;
    logfile::write("glass jaw: %s", on ? "on" : "off");
    if (!on)
        for (auto& [ped, s] : m_tracked)
            if (DOES_ENTITY_EXIST(ped)) glassJawExit(ped, s);
}

void NpcManager::settingsChanged() {
    if (m_mode == Mode::Aggressive)
        static_cast<AggressiveMode&>(current()).applyLaw(m_settings);
}

void NpcManager::exitNpc(Ped ped, TrackedNpc& s, ModeContext& ctx) {
    if (!DOES_ENTITY_EXIST(ped)) return;
    if (s.entered) current().onExit(ped, s, ctx);
    glassJawExit(ped, s);
}

void NpcManager::exitAll(ModeContext& ctx) {
    for (auto& [ped, s] : m_tracked) exitNpc(ped, s, ctx);
    m_tracked.clear();
}

void NpcManager::resetAll() {
    ModeContext ctx = makeContext(m_lastPlayer ? m_lastPlayer : PLAYER_PED_ID());
    exitAll(ctx);
    logfile::write("reset: all NPCs restored");
}

void NpcManager::glassJawTick(Ped ped, TrackedNpc& s, ModeContext& ctx, float dist) {
    const Config& c = *ctx.cfg;
    if (!s.glassEntered) {
        s.glassEntered = true;
        CLEAR_ENTITY_LAST_DAMAGE_ENTITY(ped);
    }
    if (s.eventsBlocked != ctx.settings->noRetaliation) {
        SET_BLOCKING_OF_NON_TEMPORARY_EVENTS(ped, ctx.settings->noRetaliation);
        s.eventsBlocked = ctx.settings->noRetaliation;
    }
    if (dist > 30.0f) return;

    bool hit = HAS_ENTITY_BEEN_DAMAGED_BY_ENTITY(ped, ctx.player);
    if (hit) CLEAR_ENTITY_LAST_DAMAGE_ENTITY(ped);
    bool bump = !hit && ctx.settings->bumpTrigger && dist < 3.0f && IS_ENTITY_TOUCHING_ENTITY(ped, ctx.player);
    if (!(hit || bump) || ctx.now < s.ragdollUntil) return;

    SET_PED_CAN_RAGDOLL(ped, true);
    SET_PED_TO_RAGDOLL(ped, c.ragdollMinMs, c.ragdollMaxMs, c.ragdollType);
    Vector3 pp = GET_ENTITY_COORDS(ctx.player), np = GET_ENTITY_COORDS(ped);
    float dx = np.x - pp.x, dy = np.y - pp.y;
    float len = std::sqrt(dx * dx + dy * dy);
    if (len < 0.01f) { dx = 1.0f; dy = 0.0f; len = 1.0f; }
    float f = c.pushForce;
    APPLY_FORCE_TO_ENTITY(ped, 1, dx / len * f, dy / len * f, f * 0.25f, 0.0f, 0.0f, 0.0f, 0, false, true, true, false, true);
    if (c.healAfterHit) SET_ENTITY_HEALTH(ped, GET_ENTITY_MAX_HEALTH(ped));
    s.ragdollUntil = ctx.now + static_cast<unsigned long>(c.ragdollMaxMs) + 400;
    s.hits++;
    logfile::debug("glass jaw: ped %d down (%s), hits %d", ped, hit ? "hit" : "bump", s.hits);
}

void NpcManager::glassJawExit(Ped ped, TrackedNpc& s) {
    if (s.eventsBlocked) {
        SET_BLOCKING_OF_NON_TEMPORARY_EVENTS(ped, false);
        s.eventsBlocked = false;
    }
    s.glassEntered = false;
}

void NpcManager::tick(Ped player) {
    m_lastPlayer = player;
    ModeContext ctx = makeContext(player);

    std::unordered_set<Ped> trackedSet;
    trackedSet.reserve(m_tracked.size());
    for (const auto& [ped, s] : m_tracked) trackedSet.insert(ped);

    m_scan = scanPeds(player, m_settings, m_mode, ctx.radius, ctx.exitRadius, trackedSet);

    if (m_mode == Mode::Off && !m_glassJaw) {
        if (!m_tracked.empty()) exitAll(ctx);
        return;
    }

    std::unordered_set<Ped> seen;
    auto process = [&](Ped ped, bool inside) {
        seen.insert(ped);
        TrackedNpc& s = m_tracked[ped];
        s.lastSeen = ctx.now;
        float dist = distanceTo(ped, player);
        if (!s.entered && inside) {
            s.human = IS_PED_HUMAN(ped);
            current().onEnter(ped, s, ctx);
            s.entered = true;
        }
        if (s.entered) current().onTick(ped, s, ctx, dist);
        if (m_glassJaw) glassJawTick(ped, s, ctx, dist);
    };
    for (Ped p : m_scan.inside) process(p, true);
    for (Ped p : m_scan.outside) process(p, false);

    for (auto it = m_tracked.begin(); it != m_tracked.end();) {
        if (seen.count(it->first)) { ++it; continue; }
        exitNpc(it->first, it->second, ctx);
        it = m_tracked.erase(it);
    }
}

bool NpcManager::speechTest(Ped player, std::string* info) {
    Ped best = 0;
    float bestDist = 1e9f;
    for (Ped p : m_scan.inside) {
        if (!DOES_ENTITY_EXIST(p) || !IS_PED_HUMAN(p)) continue;
        float d = distanceTo(p, player);
        if (d < bestDist) { bestDist = d; best = p; }
    }
    if (!best) {
        if (info) *info = "speech test: no NPC in range";
        return false;
    }
    bool ok = speech::say(best, "WHATS_YOUR_PROBLEM", m_cfg.insultSpeechParams, player);
    char buf[128];
    snprintf(buf, sizeof(buf), "speech test: ped %d at %.0f m -> %s", best, bestDist, ok ? "played" : "native returned 0");
    if (info) *info = buf;
    logfile::write("%s", buf);
    return ok;
}
