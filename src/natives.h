#pragma once
// Native wrappers, called by hash through the ScriptHookRDR2 SDK
// (nativeInit / nativePush64 / nativeCall from main.h).
// Hashes and signatures: RDR3 NativeDB (alloc8or), see docs/NATIVES.md.
// The generated natives.h of the 2019 SDK is not used: its names and some
// signatures are outdated (speech native, HUD text natives).

#include <windows.h>
#include <cstdint>
#include <cstring>
#include <type_traits>

#include "main.h"

using Ped = int;
using Player = int;
using Entity = int;
using Hash = uint32_t;

struct Vector3 {
    alignas(8) float x;
    alignas(8) float y;
    alignas(8) float z;
};
static_assert(sizeof(Vector3) == 24, "Vector3 must be three 8-byte slots");

namespace native {

template <typename T>
inline void push(T value) {
    static_assert(sizeof(T) <= sizeof(uint64_t), "native argument too large");
    uint64_t raw = 0;
    std::memcpy(&raw, &value, sizeof(T));
    nativePush64(raw);
}

template <typename R, typename... Args>
inline R invoke(uint64_t hash, Args... args) {
    nativeInit(hash);
    (push(args), ...);
    uint64_t* result = reinterpret_cast<uint64_t*>(nativeCall());
    if constexpr (std::is_void_v<R>) {
        return;
    } else {
        R value{};
        if (result) std::memcpy(&value, result, sizeof(R));
        return value;
    }
}

} // namespace native

// ---- PLAYER ------------------------------------------------------------------
inline Player PLAYER_ID() { return native::invoke<Player>(0x217E9DC48139933D); }
inline Ped PLAYER_PED_ID() { return native::invoke<Ped>(0x096275889B8E0EE0); }
inline bool IS_PLAYER_PLAYING(Player p) { return native::invoke<int>(0xBFFB35986CAAE58C, p) != 0; }
inline bool IS_PLAYER_CONTROL_ON(Player p) { return native::invoke<int>(0x7964097FCE4C244B, p) != 0; }
inline int GET_PLAYER_GROUP(Player p) { return native::invoke<int>(0x9BAB31815159ABCF, p); }

// ---- ENTITY ------------------------------------------------------------------
inline bool DOES_ENTITY_EXIST(Entity e) { return native::invoke<int>(0xD42BD6EB2E0F1677, e) != 0; }
inline bool IS_ENTITY_DEAD(Entity e) { return native::invoke<int>(0x7D5B1F88E7504BBA, e) != 0; }
inline Hash GET_ENTITY_MODEL(Entity e) { return native::invoke<Hash>(0xDA76A9F39210D365, e); }
inline Vector3 GET_ENTITY_COORDS(Entity e) { return native::invoke<Vector3>(0xA86D5F069399F44D, e, 1, 1); }
inline Vector3 GET_ENTITY_FORWARD_VECTOR(Entity e) { return native::invoke<Vector3>(0x2412D9C05BB09B97, e); }
inline bool IS_ENTITY_A_MISSION_ENTITY(Entity e) { return native::invoke<int>(0x138190F64DB4BBD1, e) != 0; }
inline bool IS_ENTITY_ON_SCREEN(Entity e) { return native::invoke<int>(0x613C15D5D8DB781F, e) != 0; }
inline bool HAS_ENTITY_BEEN_DAMAGED_BY_ENTITY(Entity e, Entity by) {
    return native::invoke<int>(0x7B6E7BEC1143AC86, e, by, 1, 1) != 0;
}
inline void CLEAR_ENTITY_LAST_DAMAGE_ENTITY(Entity e) { native::invoke<void>(0xBB19AC7D4DCEFD0F, e); }
inline bool HAS_ENTITY_CLEAR_LOS_TO_ENTITY(Entity a, Entity b, int traceType = 17) {
    return native::invoke<int>(0xFCDFF7B72D23A1AC, a, b, traceType) != 0;
}
inline bool IS_ENTITY_TOUCHING_ENTITY(Entity a, Entity b) { return native::invoke<int>(0x9A2304A64C3C8423, a, b) != 0; }
inline void APPLY_FORCE_TO_ENTITY(Entity e, int forceFlags, float x, float y, float z, float offX, float offY,
                                  float offZ, int boneIndex, bool isDirectionRel, bool ignoreUpVec,
                                  bool isForceRel, bool p12, bool p13) {
    native::invoke<void>(0xF15E8F5D333F09C4, e, forceFlags, x, y, z, offX, offY, offZ, boneIndex,
                         isDirectionRel ? 1 : 0, ignoreUpVec ? 1 : 0, isForceRel ? 1 : 0, p12 ? 1 : 0, p13 ? 1 : 0);
}
inline void SET_ENTITY_HEALTH(Entity e, int health, Entity killedBy = 0) { native::invoke<void>(0xAC2767ED8BDFAB15, e, health, killedBy); }
inline int GET_ENTITY_HEALTH(Entity e) { return native::invoke<int>(0x82368787EA73C0F7, e); }
inline int GET_ENTITY_MAX_HEALTH(Entity e) { return native::invoke<int>(0x15D757606D170C3C, e, 0); }

// ---- PED ---------------------------------------------------------------------
inline bool IS_PED_HUMAN(Ped p) { return native::invoke<int>(0xB980061DA992779D, p) != 0; }
inline bool IS_PED_A_PLAYER(Ped p) { return native::invoke<int>(0x12534C348C6CB68B, p) != 0; }
inline bool IS_PED_DEAD_OR_DYING(Ped p) { return native::invoke<int>(0x3317DEDB88C95038, p, 1) != 0; }
inline bool IS_PED_IN_ANY_VEHICLE(Ped p) { return native::invoke<int>(0x997ABD671D25CA0B, p, 0) != 0; }
inline bool IS_PED_ON_MOUNT(Ped p) { return native::invoke<int>(0x460BC76A0E10655E, p) != 0; }
inline Ped GET_MOUNT(Ped p) { return native::invoke<Ped>(0xE7E11B8DCBED1058, p); }
inline bool IS_PED_GROUP_MEMBER(Ped p, int groupId) { return native::invoke<int>(0x9BB01E3834671191, p, groupId, 0) != 0; }
inline bool IS_PED_IN_GROUP(Ped p) { return native::invoke<int>(0x5891CAC5D4ACFF74, p) != 0; }
inline bool IS_PED_RAGDOLL(Ped p) { return native::invoke<int>(0x47E4E977581C5B55, p) != 0; }
inline bool IS_PED_IN_COMBAT(Ped p, Ped target) { return native::invoke<int>(0x4859F1FC66A6278E, p, target) != 0; }
inline bool IS_PED_FLEEING(Ped p) { return native::invoke<int>(0xBBCCE00B381F8482, p) != 0; }
inline bool IS_PED_IN_MELEE_COMBAT(Ped p) { return native::invoke<int>(0x4E209B2C1EAD5159, p) != 0; }
inline bool IS_PED_STOPPED(Ped p) { return native::invoke<int>(0x530944F6F4B8A214, p) != 0; }
inline bool IS_PED_USING_ANY_SCENARIO(Ped p) { return native::invoke<int>(0x57AB4A3080F85143, p) != 0; }

inline Hash GET_PED_RELATIONSHIP_GROUP_HASH(Ped p) { return native::invoke<Hash>(0x7DBDD04862D95F04, p); }
inline Hash GET_PED_RELATIONSHIP_GROUP_DEFAULT_HASH(Ped p) { return native::invoke<Hash>(0x42FDD0F017B1E38E, p); }
inline void SET_PED_RELATIONSHIP_GROUP_HASH(Ped p, Hash group) { native::invoke<void>(0xC80A74AC829DDD92, p, group); }
inline bool ADD_RELATIONSHIP_GROUP(const char* name, Hash* outHash) {
    alignas(8) uint64_t h = 0;
    bool ok = native::invoke<int>(0xF372BC22FCB88606, name, &h) != 0;
    *outHash = static_cast<Hash>(h);
    return ok;
}
// relationship (RDR3 values): 0 none, 1 respect, 2 like, 3 ignore, 4 dislike, 5 wanted, 6 hate
inline void SET_RELATIONSHIP_BETWEEN_GROUPS(int relationship, Hash g1, Hash g2) {
    native::invoke<void>(0xBF25EB89375A37AD, relationship, g1, g2);
}
inline int GET_RELATIONSHIP_BETWEEN_GROUPS(Hash g1, Hash g2) { return native::invoke<int>(0x9E6B70061662AE5C, g1, g2); }

inline bool SET_PED_TO_RAGDOLL(Ped p, int timeMin, int timeMax, int ragdollType) {
    return native::invoke<int>(0xAE99FB955581844A, p, timeMin, timeMax, ragdollType, 0, 0, static_cast<const char*>(nullptr)) != 0;
}
inline void SET_PED_CAN_RAGDOLL(Ped p, bool on) { native::invoke<void>(0xB128377056A54E2A, p, on ? 1 : 0); }
inline void SET_PED_KEEP_TASK(Ped p, bool on) { native::invoke<void>(0x971D38760FBC02EF, p, on ? 1 : 0); }
// attribute: eCombatAttribute (5 CA_ALWAYS_FIGHT, 17 CA_ALWAYS_FLEE, 46 CA_CAN_FIGHT_ARMED_PEDS_WHEN_NOT_ARMED, 58 CA_DISABLE_FLEE_FROM_COMBAT)
inline void SET_PED_COMBAT_ATTRIBUTES(Ped p, int attribute, bool on) { native::invoke<void>(0x9F7794730795E019, p, attribute, on ? 1 : 0); }
inline void SET_PED_COMBAT_MOVEMENT(Ped p, int movement) { native::invoke<void>(0x4D9CA1009AFBD057, p, movement); }   // 2 = offensive
inline void SET_PED_COMBAT_ABILITY(Ped p, int level) { native::invoke<void>(0xC7622C0D36B2FDA8, p, level); }        // 0 poor, 1 average, 2 professional
inline void SET_PED_COMBAT_RANGE(Ped p, int range) { native::invoke<void>(0x3C606747B23E497B, p, range); }          // 0 near
inline void SET_PED_ACCURACY(Ped p, int accuracy) { native::invoke<void>(0x7AEFB85C1D49DEB6, p, accuracy); }
inline void SET_PED_FLEE_ATTRIBUTES(Ped p, int flags, bool on) { native::invoke<void>(0x70A2D1137C8ED7C9, p, flags, on ? 1 : 0); }
inline void SET_BLOCKING_OF_NON_TEMPORARY_EVENTS(Ped p, bool on) { native::invoke<void>(0x9F8AA94D6D97DBF4, p, on ? 1 : 0); }
inline void SET_PED_CONFIG_FLAG(Ped p, int flag, bool on) { native::invoke<void>(0x1913FE4CBF41C463, p, flag, on ? 1 : 0); }
inline void SET_PED_MOVE_RATE_OVERRIDE(Ped p, float rate) { native::invoke<void>(0x085BF80FA50A39D1, p, rate); }
inline void SET_PED_SEEING_RANGE(Ped p, float range) { native::invoke<void>(0xF29CF591C4BF6CEE, p, range); }
inline void SET_PED_HEARING_RANGE(Ped p, float range) { native::invoke<void>(0x33A8F7F7D5F7F33C, p, range); }

// Drunkness (NativeDB): SOBER 0.0, SLIGHTLY 0.25, MODERATELY 0.5, VERY 1.0
inline void _SET_PED_DRUNKNESS(Ped p, bool enabled, float level) { native::invoke<void>(0x406CCF555B04FAD3, p, enabled ? 1 : 0, level); }
inline bool _IS_PED_DRUNK(Ped p) { return native::invoke<int>(0x50F124E6EF188B22, p) != 0; }
inline float _GET_PED_DRUNKNESS(Ped p) { return native::invoke<float>(0x6FB76442469ABD68, p); }
// eMotivationState: 0 toilet, 1 fear, 2 angry, 3 agitation, 4 hungry, 5 tired, 6 sad, 7 brave, 8 offer item, 9 suspicion, 10 drunk. target 0 = everyone.
inline void _SET_PED_MOTIVATION(Ped p, int state, float threshold, Ped target) { native::invoke<void>(0x06D26A96CA1BCA75, p, state, threshold, target); }

// ---- TASK --------------------------------------------------------------------
inline void TASK_COMBAT_PED(Ped p, Ped target) { native::invoke<void>(0xF166E48407BAC484, p, target, 0, 16); }
inline void TASK_SMART_FLEE_PED(Ped p, Ped from, float distance, int timeMs, int fleeType, float speed) {
    native::invoke<void>(0x22B0D0E37CCB840D, p, from, distance, timeMs, fleeType, speed, 0);
}
inline void CLEAR_PED_TASKS(Ped p) { native::invoke<void>(0xE1EF3C1216AFF2CD, p, 1, 1); }
inline void TASK_LOOK_AT_ENTITY(Ped p, Entity target, int durationMs) { native::invoke<void>(0x69F4BE8C8CC4796C, p, target, durationMs, 2048, 3, 0); }
inline void TASK_CLEAR_LOOK_AT(Ped p) { native::invoke<void>(0x0F804F1DB19B9689, p); }
inline void TASK_TURN_PED_TO_FACE_ENTITY(Ped p, Entity target, int durationMs) {
    native::invoke<void>(0x5AD23D40115353AC, p, target, durationMs, 0.0f, 0.0f, 0.0f);
}

// ---- WEAPON ------------------------------------------------------------------
// Signature per NativeDB; p7 0.5, p8 1.0, addReason ADD_REASON_DEFAULT (0x2CD419DC) like the R* scripts.
inline void GIVE_WEAPON_TO_PED(Ped p, Hash weapon, int ammo, bool inHand, Hash addReason) {
    native::invoke<Hash>(0x5E3BDDBCB83F3D84, p, weapon, ammo, inHand ? 1 : 0, 0, 0, 0, 0.5f, 1.0f, addReason, 1, 0.0f, 0);
}
inline void REMOVE_WEAPON_FROM_PED(Ped p, Hash weapon) { native::invoke<void>(0x4899CB088EDF59B8, p, weapon, 1, 0); }
inline bool HAS_PED_GOT_WEAPON(Ped p, Hash weapon) { return native::invoke<int>(0x8DECB02F88F428BC, p, weapon, 0, 0) != 0; }
inline void SET_PED_INFINITE_AMMO(Ped p, bool on, Hash weapon) { native::invoke<void>(0x3EDCB0505123623B, p, on ? 1 : 0, weapon); }

// ---- AUDIO -------------------------------------------------------------------
// PLAY_PED_AMBIENT_SPEECH_NATIVE: the ScriptedSpeechParams struct lives in speech.cpp.
inline bool PLAY_PED_AMBIENT_SPEECH_NATIVE(Ped p, void* params) { return native::invoke<int>(0x8E04FEDD28D42462, p, params) != 0; }
inline bool DOES_CONTEXT_EXIST_FOR_THIS_PED(Ped p, const char* speechName) { return native::invoke<int>(0x49B99BF3FDA89A7A, p, speechName, 0) != 0; }
inline bool IS_ANY_SPEECH_PLAYING(Ped p) { return native::invoke<int>(0x729072355FA39EC9, p) != 0; }
inline void SET_PED_IS_DRUNK(Ped p, bool on) { native::invoke<void>(0x95D2D383D5396B8A, p, on ? 1 : 0); }
inline void PLAY_SOUND_FRONTEND(const char* name, const char* set) { native::invoke<void>(0x67C540AA08E4A6F5, name, set, 1, 0); }

// ---- EXPERIMENTAL: robbery ---------------------------------------------------
inline void TASK_AIM_GUN_AT_ENTITY(Ped p, Entity target, int durationMs) { native::invoke<void>(0x9B53BB6E8943AF53, p, target, durationMs, 0, 0); }
// flags: 0 = HANDS_UP_NOTHING, 1 = HANDS_UP_STRAIGHT_TO_LOOP
inline void TASK_HANDS_UP(Ped p, int durationMs, Ped facing, int timeToFaceMs, int flags) {
    native::invoke<void>(0xF2EAB31979A7F910, p, durationMs, facing, timeToFaceMs, flags);
}
inline void TASK_WANDER_STANDARD(Ped p) { native::invoke<void>(0xBB9CE077274F6A1B, p, 10.0f, 10); }
inline void TASK_STAND_STILL(Ped p, int durationMs) { native::invoke<void>(0x919BE13EED931959, p, durationMs); }
inline void SET_CURRENT_PED_WEAPON(Ped p, Hash weapon, bool equipNow) { native::invoke<void>(0xADF692B254977C0C, p, weapon, equipNow ? 1 : 0, 0, 0, 0); }
inline void TASK_GO_TO_ENTITY(Ped p, Entity target, int durationMs, float stopDistance, float speed) {
    native::invoke<void>(0x6A071245EB0D1882, p, target, durationMs, stopDistance, speed, 0.0f, 0);
}
// 0 WAITING_TO_START, 1 PERFORMING, 2 DORMANT, 3 VACANT, 7 TASK_NOT_FOUND, 8 FINISHED (eScriptTaskHash in docs)
inline int GET_SCRIPT_TASK_STATUS(Ped p, Hash scriptTask) { return native::invoke<int>(0x77F1BEB8863288D5, p, scriptTask, 0); }

// ---- EXPERIMENTAL: lasso -----------------------------------------------------
inline void TASK_LASSO_PED(Ped p, Ped target) { native::invoke<void>(0xC716EB2BD16370A3, p, target); }
inline void TASK_HOGTIE_TARGET_PED(Ped p, Ped target) { native::invoke<void>(0x27829AFD3E03AC1A, p, target); }
inline bool IS_PED_LASSOED(Ped p) { return native::invoke<int>(0x9682F850056C9ADE, p) != 0; }
inline bool IS_PED_HOGTIED(Ped p) { return native::invoke<int>(0x3AA24CCC0D451379, p) != 0; }
inline bool IS_PED_BEING_HOGTIED(Ped p) { return native::invoke<int>(0xD453BB601D4A606E, p) != 0; }
inline void _SET_PED_CAN_BE_LASSOED(Ped p, bool on) { native::invoke<void>(0xFD6943B6DF77E449, p, on ? 1 : 0); }
// flags (femga): 0 CAN_BE_LASSOED, 2 CAN_BE_LASSOED_BY_FRIENDLY_AI, 4 CAN_RUN_PRIMARY_LASSO_REACT_TASK,
// 5 SHOULD_FLEE_ON_ATTACH, 6 SHOULD_ATTACK_ON_ATTACH, 7 CAN_BE_REELED_IN
inline void SET_PED_LASSO_HOGTIE_FLAG(Ped p, int flag, bool on) { native::invoke<void>(0xAE6004120C18DF97, p, flag, on ? 1 : 0); }
inline void _SET_HOGTIE_ESCAPE_TIMER(Ped p, float seconds) { native::invoke<void>(0xAB591AE6B48B913E, p, seconds); }

// ---- LAW ---------------------------------------------------------------------
inline void _SET_LAW_DISABLED(bool on) { native::invoke<void>(0x8DE82BC774F3B862, on ? 1 : 0); }

// ---- MISC / SCRIPT -----------------------------------------------------------
inline Hash GET_HASH_KEY(const char* s) { return native::invoke<Hash>(0xFD340785ADF8CFB7, s); }
inline bool IS_PAUSE_MENU_ACTIVE() { return native::invoke<int>(0x535384D6067BA42E) != 0; }
inline bool IS_LOADING_SCREEN_VISIBLE() { return native::invoke<int>(0xB54ADBE65D528FCB) != 0; }
inline bool IS_SCREEN_FADED_OUT() { return native::invoke<int>(0xF5472C80DF2FF847) != 0; }
inline bool NETWORK_IS_SESSION_STARTED() { return native::invoke<int>(0x9DE624D2FC4B603F) != 0; }

// ---- HUD ---------------------------------------------------------------------
// VAR_STRING(10, "LITERAL_STRING", text) wraps a plain C string for the UI natives.
inline const char* VAR_STRING_LITERAL(const char* text) {
    return native::invoke<const char*>(0xFA925AC00EB830B9, 10, "LITERAL_STRING", text);
}
// Current natives (UIDEBUG namespace), the way to draw text since build 1436.
inline void _BG_SET_TEXT_SCALE(float sx, float sy) { native::invoke<void>(0xA1253A3C870B6843, sx, sy); }
inline void _BG_SET_TEXT_COLOR(int r, int g, int b, int a) { native::invoke<void>(0x16FA5CE47F184F1E, r, g, b, a); }
inline void _BG_DISPLAY_TEXT(const char* varString, float x, float y) { native::invoke<void>(0x16794E044C9EFB58, varString, x, y); }
// Legacy pair (the 2019 SDK sample uses these). Fallback: TextBackend=legacy in the ini.
inline void _SET_TEXT_COLOR_LEGACY(int r, int g, int b, int a) { native::invoke<void>(0x50A41AD966910F03, r, g, b, a); }
inline void _DISPLAY_TEXT_LEGACY(const char* varString, float x, float y) { native::invoke<void>(0xD79334A4BB99BAD1, varString, x, y); }
// x/y = centre, w/h relative to the screen (0..1)
inline void DRAW_RECT(float x, float y, float w, float h, int r, int g, int b, int a) {
    native::invoke<void>(0x405224591DF02025, x, y, w, h, r, g, b, a, 0, 0);
}
