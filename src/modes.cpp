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

// ---- experimental: helpers for pair modes ------------------------------------------

// Nearest free, living, human NPC within maxDist that passes extraOk.
template <typename Ok>
static Ped findPartner(Ped self, ModeContext& ctx, float maxDist, Ok extraOk) {
    Ped best = 0;
    float bestDist = maxDist;
    Vector3 me = GET_ENTITY_COORDS(self);
    for (auto& [other, t] : *ctx.tracked) {
        if (other == self || !t.entered || !t.human || t.role != 0 || ctx.now < t.cooldownUntil) continue;
        if (!DOES_ENTITY_EXIST(other) || IS_ENTITY_DEAD(other) || IS_PED_RAGDOLL(other) || IS_PED_ON_MOUNT(other)) continue;
        if (!extraOk(other)) continue;
        float d = distanceBetween(me, GET_ENTITY_COORDS(other));
        if (d < bestDist) { bestDist = d; best = other; }
    }
    return best;
}

static TrackedNpc* trackedOf(ModeContext& ctx, Ped ped) {
    auto it = ctx.tracked->find(ped);
    return it == ctx.tracked->end() ? nullptr : &it->second;
}

static bool alive(Ped ped) { return ped && DOES_ENTITY_EXIST(ped) && !IS_ENTITY_DEAD(ped); }

static void freeRole(TrackedNpc& s, unsigned long until) {
    s.role = 0;
    s.partner = 0;
    s.phase = 0;
    s.phaseUntil = 0;
    s.cooldownUntil = until;
}

// ---- Robbery (experimental) -----------------------------------------------------------
// Pairs: a robber draws a gun on a victim, the victim raises the hands, hands over the
// money and runs; then both are free again and may rob or be robbed next.

struct RobberyMode : NpcMode {
    Mode id() const override { return Mode::Robbery; }

    void onEnter(Ped, TrackedNpc& s, ModeContext& ctx) override {
        freeRole(s, ctx.now + static_cast<unsigned long>(ctx.randRange(500, 4000)));
    }

    void finish(Ped robber, TrackedNpc& r, Ped victim, TrackedNpc* v, ModeContext& ctx, bool aborted) {
        const Config& c = *ctx.cfg;
        if (alive(robber)) {
            CLEAR_PED_TASKS(robber);
            SET_CURRENT_PED_WEAPON(robber, joaat("WEAPON_UNARMED"), true);
            SET_BLOCKING_OF_NON_TEMPORARY_EVENTS(robber, false);
            if (!aborted) TASK_WANDER_STANDARD(robber);
        }
        r.eventsBlocked = false;
        freeRole(r, ctx.now + ctx.randMs(c.robCooldownMinSec, c.robCooldownMaxSec));
        if (v) {
            if (alive(victim)) {
                CLEAR_PED_TASKS(victim);
                SET_BLOCKING_OF_NON_TEMPORARY_EVENTS(victim, false);
                if (!aborted) {
                    TASK_SMART_FLEE_PED(victim, robber, 40.0f, 8000, 0, 3.0f);
                    if (speech::globalSlot(ctx.now, 300)) speech::sayOneOf(victim, c.robFleeLines, c.robSpeechParams, robber);
                }
            }
            v->eventsBlocked = false;
            freeRole(*v, ctx.now + ctx.randMs(c.robCooldownMinSec, c.robCooldownMaxSec));
        }
        logfile::write("robbery: %d robbed %d %s", robber, victim, aborted ? "(aborted)" : "(done)");
    }

    void onTick(Ped ped, TrackedNpc& s, ModeContext& ctx, float) override {
        const Config& c = *ctx.cfg;
        if (!s.human || s.role == 2) return;   // victims are driven by their robber

        if (s.role == 1) {
            TrackedNpc* v = trackedOf(ctx, s.partner);
            if (!v || v->partner != ped || !alive(s.partner) || !alive(ped)) { finish(ped, s, s.partner, v, ctx, true); return; }
            if (v->nextSpeech && ctx.now >= v->nextSpeech) {   // the victim answers a moment after the threat
                if (speech::globalSlot(ctx.now, 300)) speech::sayOneOf(s.partner, c.robVictimLines, c.robSpeechParams, ped);
                v->nextSpeech = 0;
            }
            if (ctx.now < s.phaseUntil) return;
            if (s.phase == 1) {   // hold-up over: hand over the money
                if (speech::globalSlot(ctx.now, 300)) speech::sayOneOf(s.partner, c.robHandOverLines, c.robSpeechParams, ped);
                s.phase = 2;
                s.phaseUntil = ctx.now + 2500;
                return;
            }
            finish(ped, s, s.partner, v, ctx, false);
            return;
        }

        // free: look for a victim
        if (ctx.now < s.cooldownUntil || IS_PED_RAGDOLL(ped) || IS_PED_ON_MOUNT(ped) || IS_PED_IN_COMBAT(ped, ctx.player) || IS_PED_FLEEING(ped)) return;
        Ped victim = findPartner(ped, ctx, c.robDistance, [](Ped) { return true; });
        if (!victim) { s.cooldownUntil = ctx.now + 2000; return; }
        TrackedNpc& v = (*ctx.tracked)[victim];

        s.role = 1; s.partner = victim; s.phase = 1;
        s.phaseUntil = ctx.now + static_cast<unsigned long>(c.robHoldupSec) * 1000;
        v.role = 2; v.partner = ped; v.phase = 1;
        v.nextSpeech = ctx.now + 1200;

        SET_BLOCKING_OF_NON_TEMPORARY_EVENTS(ped, true); s.eventsBlocked = true;
        SET_BLOCKING_OF_NON_TEMPORARY_EVENTS(victim, true); v.eventsBlocked = true;
        Hash w = joaat(c.robWeapon);
        if (!HAS_PED_GOT_WEAPON(ped, w)) { GIVE_WEAPON_TO_PED(ped, w, 30, true, kAddReasonDefault); s.weaponGiven = w; }
        else SET_CURRENT_PED_WEAPON(ped, w, true);
        int holdMs = c.robHoldupSec * 1000 + 4000;
        TASK_AIM_GUN_AT_ENTITY(ped, victim, holdMs);
        TASK_HANDS_UP(victim, holdMs, ped, 1000, 1);
        if (speech::globalSlot(ctx.now, 300)) speech::sayOneOf(ped, c.robRobberLines, c.robSpeechParams, victim);
        logfile::write("robbery: %d holds up %d", ped, victim);
    }

    void onExit(Ped ped, TrackedNpc& s, ModeContext& ctx) override {
        if (s.role == 1) {
            finish(ped, s, s.partner, trackedOf(ctx, s.partner), ctx, true);
        } else if (s.role == 2) {
            TrackedNpc* r = trackedOf(ctx, s.partner);
            if (r && r->partner == ped) finish(s.partner, *r, ped, &s, ctx, true);
        }
        if (!alive(ped)) return;
        if (s.eventsBlocked) { SET_BLOCKING_OF_NON_TEMPORARY_EVENTS(ped, false); s.eventsBlocked = false; }
        if (s.weaponGiven) { REMOVE_WEAPON_FROM_PED(ped, s.weaponGiven); s.weaponGiven = 0; }
        CLEAR_PED_TASKS(ped);
    }
};

// ---- Lasso (experimental, second attempt) ----------------------------------------------
// Everyone gets a lasso in the hand. Pairs: the thrower walks up to the target, throws
// (TASK_LASSO_PED), hogties the catch; the catch breaks free after a few seconds and may
// take revenge next round. Every step and the script task status go to the log.

struct LassoMode : NpcMode {
    const Hash lasso = joaat("WEAPON_LASSO");
    static constexpr Hash kScriptTaskLasso = 0x03E3E84F;    // SCRIPT_TASK_LASSO_PED
    static constexpr Hash kScriptTaskHogtie = 0x5EAF7691;   // SCRIPT_TASK_HOGTIE_TARGET_PED

    Mode id() const override { return Mode::Lasso; }

    void onEnter(Ped ped, TrackedNpc& s, ModeContext& ctx) override {
        if (!s.human) return;
        if (!HAS_PED_GOT_WEAPON(ped, lasso)) { GIVE_WEAPON_TO_PED(ped, lasso, 1, true, kAddReasonDefault); s.weaponGiven = lasso; }
        SET_CURRENT_PED_WEAPON(ped, lasso, true);
        _SET_PED_CAN_BE_LASSOED(ped, true);
        SET_PED_LASSO_HOGTIE_FLAG(ped, 0, true);   // can be lassoed
        SET_PED_LASSO_HOGTIE_FLAG(ped, 2, true);   // by friendly AI
        SET_PED_LASSO_HOGTIE_FLAG(ped, 4, true);   // runs the lasso react task
        SET_PED_LASSO_HOGTIE_FLAG(ped, 7, true);   // can be reeled in
        SET_PED_LASSO_HOGTIE_FLAG(ped, 6, false);  // no attack on attach
        freeRole(s, ctx.now + static_cast<unsigned long>(ctx.randRange(500, 4000)));
    }

    void finish(Ped lassoer, TrackedNpc& l, Ped target, TrackedNpc* t, ModeContext& ctx, const char* why) {
        const Config& c = *ctx.cfg;
        if (alive(lassoer)) {
            SET_PED_KEEP_TASK(lassoer, false);
            CLEAR_PED_TASKS(lassoer);
            TASK_WANDER_STANDARD(lassoer);
        }
        freeRole(l, ctx.now + ctx.randMs(c.lassoCooldownMinSec, c.lassoCooldownMaxSec));
        if (t) {
            if (alive(target)) {
                if (IS_PED_HOGTIED(target) || IS_PED_BEING_HOGTIED(target)) _SET_HOGTIE_ESCAPE_TIMER(target, 1.0f);
                else if (!IS_PED_LASSOED(target)) CLEAR_PED_TASKS(target);
            }
            // the catch gets a short break and may lasso back
            freeRole(*t, ctx.now + static_cast<unsigned long>(ctx.randRange(2000, 6000)));
        }
        logfile::write("lasso: %d -> %d %s", lassoer, target, why);
    }

    void startPhase(TrackedNpc& s, int phase, unsigned long now, unsigned long durationMs) {
        s.phase = phase;
        s.phaseStart = now;
        s.phaseUntil = now + durationMs;
        s.lastTaskStatus = -1;
    }

    void logStatus(Ped ped, TrackedNpc& s, Hash task, const char* what) {
        int status = GET_SCRIPT_TASK_STATUS(ped, task);
        if (status != s.lastTaskStatus) {
            logfile::write("lasso: %d %s task status %d", ped, what, status);
            s.lastTaskStatus = status;
        }
    }

    void onTick(Ped ped, TrackedNpc& s, ModeContext& ctx, float) override {
        const Config& c = *ctx.cfg;
        if (!s.human || s.role == 2) return;

        if (s.role == 1) {
            TrackedNpc* t = trackedOf(ctx, s.partner);
            Ped target = s.partner;
            if (!t || t->partner != ped || !alive(target) || !alive(ped)) { finish(ped, s, target, t, ctx, "(partner lost)"); return; }
            float d = distanceTo(ped, target);

            if (s.phase == 0) {   // walking up
                if (d <= c.lassoThrowDistance) {
                    SET_CURRENT_PED_WEAPON(ped, lasso, true);
                    TASK_LASSO_PED(ped, target);
                    SET_PED_KEEP_TASK(ped, true);
                    if (c.lassoTargetFlees) TASK_SMART_FLEE_PED(target, ped, 30.0f, c.lassoTimeoutSec * 1000, 0, c.lassoFleeSpeed);
                    startPhase(s, 1, ctx.now, static_cast<unsigned long>(c.lassoTimeoutSec) * 1000);
                    logfile::write("lasso: %d throws at %d (%.1f m)", ped, target, d);
                } else if (ctx.now >= s.phaseUntil) {
                    finish(ped, s, target, t, ctx, "(never got close)");
                }
                return;
            }
            if (s.phase == 1) {   // throwing
                logStatus(ped, s, kScriptTaskLasso, "lasso");
                if (IS_PED_LASSOED(target)) {
                    logfile::write("lasso: %d caught %d", ped, target);
                    if (speech::globalSlot(ctx.now, 300)) speech::sayOneOf(target, c.lassoTargetLines, c.lassoSpeechParams, ped);
                    if (c.lassoHogtie) {
                        TASK_HOGTIE_TARGET_PED(ped, target);
                        if (speech::globalSlot(ctx.now, 300)) speech::sayOneOf(ped, c.lassoLassoerLines, c.lassoSpeechParams, target);
                        startPhase(s, 2, ctx.now, 15000);
                    } else {
                        startPhase(s, 3, ctx.now, static_cast<unsigned long>(c.lassoHogtieHoldSec) * 1000);
                    }
                } else if (ctx.now >= s.phaseUntil) {
                    finish(ped, s, target, t, ctx, "(gave up, no catch)");
                } else if (s.lastTaskStatus == 7 && ctx.now - s.phaseStart > 2500 && d > c.lassoThrowDistance + 3.0f) {
                    // the task was dropped (status 7 = not found) and the target walked off: chase and retry
                    TASK_GO_TO_ENTITY(ped, target, 8000, c.lassoThrowDistance - 1.0f, 2.0f);
                    startPhase(s, 0, ctx.now, 10000);
                    logfile::write("lasso: %d lost the throw, walks after %d again", ped, target);
                }
                return;
            }
            if (s.phase == 2) {   // hogtying
                logStatus(ped, s, kScriptTaskHogtie, "hogtie");
                if (IS_PED_HOGTIED(target)) {
                    _SET_HOGTIE_ESCAPE_TIMER(target, static_cast<float>(c.lassoHogtieHoldSec));
                    startPhase(s, 3, ctx.now, static_cast<unsigned long>(c.lassoHogtieHoldSec) * 1000 + 3000);
                    logfile::write("lasso: %d hogtied %d", ped, target);
                } else if (ctx.now >= s.phaseUntil) {
                    finish(ped, s, target, t, ctx, "(hogtie failed)");
                }
                return;
            }
            // phase 3: holding; done when the timer runs out or the catch broke free
            if (ctx.now >= s.phaseUntil || (c.lassoHogtie && !IS_PED_HOGTIED(target) && !IS_PED_BEING_HOGTIED(target)))
                finish(ped, s, target, t, ctx, "(done)");
            return;
        }

        // free: pick a target
        if (ctx.now < s.cooldownUntil || IS_PED_RAGDOLL(ped) || IS_PED_ON_MOUNT(ped) || IS_PED_LASSOED(ped) ||
            IS_PED_HOGTIED(ped) || IS_PED_IN_COMBAT(ped, ctx.player))
            return;
        Ped target = findPartner(ped, ctx, c.lassoDistance, [](Ped p) { return !IS_PED_LASSOED(p) && !IS_PED_HOGTIED(p); });
        if (!target) { s.cooldownUntil = ctx.now + 2000; return; }
        TrackedNpc& t = (*ctx.tracked)[target];

        s.role = 1; s.partner = target;
        t.role = 2; t.partner = ped; t.phase = 1;
        float d = distanceTo(ped, target);
        if (d > c.lassoThrowDistance) TASK_GO_TO_ENTITY(ped, target, c.lassoTimeoutSec * 1000, c.lassoThrowDistance - 1.0f, 2.0f);
        startPhase(s, 0, ctx.now, static_cast<unsigned long>(c.lassoTimeoutSec) * 1000);
        logfile::write("lasso: %d picks %d (%.1f m)", ped, target, d);
    }

    void onExit(Ped ped, TrackedNpc& s, ModeContext& ctx) override {
        if (s.role == 1) {
            finish(ped, s, s.partner, trackedOf(ctx, s.partner), ctx, "(mode off)");
        } else if (s.role == 2) {
            TrackedNpc* l = trackedOf(ctx, s.partner);
            if (l && l->partner == ped) finish(s.partner, *l, ped, &s, ctx, "(mode off)");
        }
        if (!alive(ped)) return;
        SET_PED_LASSO_HOGTIE_FLAG(ped, 2, false);
        if (IS_PED_HOGTIED(ped) || IS_PED_BEING_HOGTIED(ped)) _SET_HOGTIE_ESCAPE_TIMER(ped, 0.5f);
        if (s.weaponGiven) { REMOVE_WEAPON_FROM_PED(ped, s.weaponGiven); s.weaponGiven = 0; }
        else SET_CURRENT_PED_WEAPON(ped, joaat("WEAPON_UNARMED"), true);
        SET_PED_KEEP_TASK(ped, false);
        if (!IS_PED_HOGTIED(ped)) CLEAR_PED_TASKS(ped);
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
    m_modes[static_cast<int>(Mode::Robbery)] = std::make_unique<RobberyMode>();
    m_modes[static_cast<int>(Mode::Lasso)] = std::make_unique<LassoMode>();
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
    ctx.tracked = &m_tracked;
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
