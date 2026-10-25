# Notes on the natives used

Sources:
- [RDR3 NativeDB](https://alloc8or.re/rdr3/nativedb/) (raw data: `alloc8or/rdr3-nativedb-data`)
- [femga/rdr3_discoveries](https://github.com/femga/rdr3_discoveries): `AI/RELATIONSHIP`, `AI/FLEE_ATTRIBUTES`,
  `AI/COMBAT_ATTRIBUTES`, `AI/CPED_CONFIG_FLAGS`, `AI/LASSO_HOGTIE_FLAG`, `audio/audio_banks`,
  `peds/peds_list.lua`, `weapons/weapons.lua`
- [rdr2mods.com speech line database](https://www.rdr2mods.com/wiki/speechlines/) and the thread
  [Making NPCs speak (solved)](https://www.rdr2mods.com/forums/topic/467-making-npcs-speak-solved/)
- [alloc8or eCombatAttribute](https://alloc8or.re/rdr3/doc/enums/eCombatAttribute.txt),
  [eScriptTaskHash](https://alloc8or.re/rdr3/doc/enums/eScriptTaskHash.txt)
- Game build 1491.17 with ScriptHookRDR2 from February 2023

## Which peds may be touched

| Check | Native | Hash |
|---|---|---|
| all peds | `worldGetAllPeds` (SDK) | - |
| player | `PLAYER_PED_ID`, `IS_PED_A_PLAYER` | `0x096275889B8E0EE0`, `0x12534C348C6CB68B` |
| player's mount | `IS_PED_ON_MOUNT`, `GET_MOUNT` | `0x460BC76A0E10655E`, `0xE7E11B8DCBED1058` |
| dead / dying | `IS_ENTITY_DEAD`, `IS_PED_DEAD_OR_DYING` | `0x7D5B1F88E7504BBA`, `0x3317DEDB88C95038` |
| human | `IS_PED_HUMAN` | `0xB980061DA992779D` |
| **story protection** | `IS_ENTITY_A_MISSION_ENTITY` | `0x138190F64DB4BBD1` |
| vehicle | `IS_PED_IN_ANY_VEHICLE` | `0x997ABD671D25CA0B` |
| player group | `GET_PLAYER_GROUP`, `IS_PED_GROUP_MEMBER` | `0x9BAB31815159ABCF`, `0x9BB01E3834671191` |
| default relationship group | `GET_PED_RELATIONSHIP_GROUP_DEFAULT_HASH` | `0x42FDD0F017B1E38E` |

Protected groups (joaat of the femga names): `PLAYER`, `REL_GANG_DUTCHS`, `REL_GANG_DUTCHS_HORSES`,
`REL_PLAYER_ALLY`, `REL_COMPANION_GROUP`, `REL_PLAYER_LIKE`.
Law and guards: `COP`, `REL_COP`, `REL_PLAYER_COP`, `REL_PINKERTONS`, `REL_GUAMA_LAW`, `REL_BOUNTY_HUNTER`,
`REL_GUARD*`, `REL_GREYS_GUARD`, `REL_BRAITH_GUARD` **plus** a model list, because the ambient lawmen
(`s_m_m_ambientlawrural_01`, `u_m_m_valsheriff_01`, ...) sit in `REL_CIVMALE` according to femga.

## Relationship types (RDR3, not the GTA V values)

| Id | Name |
|---|---|
| 0 | NONE |
| 1 | RESPECT |
| 2 | LIKE |
| 3 | IGNORE |
| 4 | DISLIKE |
| 5 | WANTED |
| **6** | **HATE** |
| 7 | DEAD |

`SET_RELATIONSHIP_BETWEEN_GROUPS(6, hostile, player)` in both directions; `ADD_RELATIONSHIP_GROUP`
(`0xF372BC22FCB88606`) returns the hash through its out parameter.

## Speech

`AUDIO::PLAY_PED_AMBIENT_SPEECH_NATIVE(Ped, ScriptedSpeechParams*)` `0x8E04FEDD28D42462` (old name
`_PLAY_AMBIENT_SPEECH1`; the 2019 SDK header has a wrong two-parameter signature).

```cpp
struct ScriptedSpeechParams {          // 0x40 bytes
    const char* speechName;            // e.g. "GENERIC_INSULT_HIGH"
    const char* voiceName;             // nullptr = own voice, or a voice bank name
    alignas(8) int variation;          // 0 = random line of that context
    alignas(8) Hash speechParamHash;   // offset 24
    alignas(8) Ped listenerPed;
    alignas(8) BOOL syncOverNetwork;   // 1
    alignas(8) int v7, v8;             // 1, 1 (mission scripts set f_5/f_6 = 1)
};
```

Speech params (joaat of `SPEECH_PARAMS_<NAME>`, table from femga `audio_banks`):
`force` 0xF85DD899 (used for insults and robbery), `force_shouted_critical` 0xFBDA0604 (R* uses it for PLEAD;
used for panic, drunk, lasso), `beat_shouted_clear` 0x67F3AB43, `force_shouted_clear` 0x9DE2F602,
`standard` 0x3CA9FB81, `shouted` 0xC3159875, `force_normal` 0x5BDE1482, `force_normal_clear` 0xA7E68047.
0xE7176FB2 (worked in the community test) matches no known name and is available as `lms` in the ini.

Speech lines confirmed in the rdr2mods database (number of voices): `GENERIC_INSULT_HIGH` (50),
`GENERIC_INSULT_MED` (50), `GENERIC_CURSE_HIGH` (700), `GENERIC_CURSE_MED` (700), `GET_LOST` (143),
`WHATS_YOUR_PROBLEM` (717), `CHALLENGE_THREATEN` (617), `CIVILIAN_TAUNT_MED` (138), `PANIC_HELP` (490),
`SCARED_HELP` (473), `GENERIC_FRIGHTENED_HIGH` (694), `GENERIC_SHOCKED_HIGH` (737), `PLEAD` (710), `DRUNK` (366).
Present in the context list only: `GENERIC_ANGRY_SHOUT`, `DISMISSIVE_REACT_ANNOYED`, `SCREAM_TERROR`,
`SCREAM_SHOCKED`, `GUN_RUN`, `CHALLENGE_THREATEN_DRUNK`, `CHALLENGE_THREATEN_ARMED`, `HANDS_UP`,
`HOLDUP_DONT_SHOOT`, `HOLDUP_DONT_KILL_ME`, `HOLDUP_GIVE_CASH`, `HOLDUP_RUN`, `HOLDUP_HELP`, `HAND_OVER_MONEY`,
`INTIMIDATED_ROB`, `HOGTIES_MALE`, `HOGTIES_FEMALE`. **`ANTAGONIZE` does not exist.**
`DOES_CONTEXT_EXIST_FOR_THIS_PED` `0x49B99BF3FDA89A7A` checks whether the voice has the line.

## Drunk

- `PED::_SET_PED_DRUNKNESS(Ped, BOOL enabled, float level)` `0x406CCF555B04FAD3`: SOBER 0.0, SLIGHTLY 0.25,
  MODERATELY 0.5, VERY 1.0. Also `_IS_PED_DRUNK` `0x50F124E6EF188B22`, `_GET_PED_DRUNKNESS` `0x6FB76442469ABD68`.
- `AUDIO::SET_PED_IS_DRUNK` `0x95D2D383D5396B8A` (drunk voice variants).
- `PED::_SET_PED_MOTIVATION(Ped, int state, float threshold, Ped target)` `0x06D26A96CA1BCA75`,
  eMotionState: 0 TOILET, 1 FEAR, 2 ANGRY, 3 AGITATION, ..., 10 DRUNK (optional, `UseMotivation=1`).
- Stumbling: short ragdoll via `SET_PED_TO_RAGDOLL` `0xAE99FB955581844A`.

## Aggressive

- `TASK_COMBAT_PED(Ped, Ped, int, int)` `0xF166E48407BAC484`, R* values `(..., 0, 16)`.
- `SET_PED_COMBAT_ATTRIBUTES` `0x9F7794730795E019`: 5 ALWAYS_FIGHT, 17 ALWAYS_FLEE,
  46 CAN_FIGHT_ARMED_PEDS_WHEN_NOT_ARMED, 58 DISABLE_FLEE_FROM_COMBAT (alloc8or eCombatAttribute).
- `SET_PED_COMBAT_MOVEMENT` 2 = offensive, `SET_PED_COMBAT_ABILITY` 0/1/2, `SET_PED_COMBAT_RANGE` 0 = near,
  `SET_PED_ACCURACY` 0-100.
- `GIVE_WEAPON_TO_PED(Ped, Hash, int ammo, BOOL inHand, BOOL inHolster, int attachPoint, BOOL multiple,
  float 0.5, float 1.0, Hash addReason, BOOL ignoreUnlocks, float degradation, BOOL)` `0x5E3BDDBCB83F3D84`,
  `addReason = joaat("ADD_REASON_DEFAULT") = 0x2CD419DC`. Weapons: `WEAPON_MELEE_KNIFE` 0xDB21AC8C,
  `WEAPON_REVOLVER_CATTLEMAN` 0x169F59F7, `WEAPON_LASSO` 0x7A8A724A (femga weapons.lua).
- `LAW::_SET_LAW_DISABLED(BOOL)` `0x8DE82BC774F3B862` for "Disable law".

## Glass jaw

`HAS_ENTITY_BEEN_DAMAGED_BY_ENTITY(e1, e2, 1, 1)` `0x7B6E7BEC1143AC86` -> `CLEAR_ENTITY_LAST_DAMAGE_ENTITY`
`0xBB19AC7D4DCEFD0F` -> `SET_PED_CAN_RAGDOLL` `0xB128377056A54E2A` -> `SET_PED_TO_RAGDOLL(ped, min, max, type, 0, 0, NULL)`
-> `APPLY_FORCE_TO_ENTITY` `0xF15E8F5D333F09C4` -> `SET_ENTITY_HEALTH(ped, GET_ENTITY_MAX_HEALTH(ped, 0), 0)`
(`0xAC2767ED8BDFAB15`, `0x15D757606D170C3C`). Bump trigger: `IS_ENTITY_TOUCHING_ENTITY` `0x9A2304A64C3C8423`.
No retaliation: `SET_BLOCKING_OF_NON_TEMPORARY_EVENTS` `0x9F8AA94D6D97DBF4`.

## Panic

`TASK_SMART_FLEE_PED(Ped, Ped from, float distance, int time, int fleeType, float speed, Ped)` `0x22B0D0E37CCB840D`
(time -1 = forever, speed 3.0 like R*), `SET_PED_KEEP_TASK` `0x971D38760FBC02EF`, `IS_PED_FLEEING` `0xBBCCE00B381F8482`.
Flee attributes (femga): 65536 FORCE_EXIT_VEHICLE, 1048576 DISABLE_MOUNT_USAGE, 4194304 DISABLE_ENTER_VEHICLES.

## Robbery

Pairs are formed from the tracked NPCs (robber / victim, state kept per ped).
- Robber: `GIVE_WEAPON_TO_PED` (revolver in hand) or `SET_CURRENT_PED_WEAPON` `0xADF692B254977C0C`,
  `TASK_AIM_GUN_AT_ENTITY(Ped, Entity, int duration, BOOL, int)` `0x9B53BB6E8943AF53`.
- Victim: `TASK_HANDS_UP(Ped, int duration, Ped facing, int timeToFace, int flags)` `0xF2EAB31979A7F910`
  (flags 1 = straight to loop).
- Both: `SET_BLOCKING_OF_NON_TEMPORARY_EVENTS` during the hold-up so neither runs off.
- End: robber holsters (`SET_CURRENT_PED_WEAPON(WEAPON_UNARMED)`) and `TASK_WANDER_STANDARD` `0xBB9CE077274F6A1B`;
  victim `TASK_SMART_FLEE_PED` for a few seconds.
- Also available but not used: `_TASK_INTIMIDATED_2` `0x933ACC1A1771A288`, `TASK_LOOT_ENTITY` `0x48FAE038401A2888`.

## Lasso

- Weapon `WEAPON_LASSO` 0x7A8A724A, equipped with `SET_CURRENT_PED_WEAPON`.
- `_SET_PED_CAN_BE_LASSOED` `0xFD6943B6DF77E449`, `SET_PED_LASSO_HOGTIE_FLAG(Ped, int flag, BOOL)` `0xAE6004120C18DF97`
  with femga flags: 0 CAN_BE_LASSOED, 2 CAN_BE_LASSOED_BY_FRIENDLY_AI, 4 CAN_RUN_PRIMARY_LASSO_REACT_TASK,
  5 SHOULD_FLEE_ON_ATTACH, 6 SHOULD_ATTACK_ON_ATTACH, 7 CAN_BE_REELED_IN.
- `TASK_LASSO_PED(Ped, Ped)` `0xC716EB2BD16370A3`; progress via `GET_SCRIPT_TASK_STATUS(Ped, Hash, BOOL)`
  `0x77F1BEB8863288D5` with `SCRIPT_TASK_LASSO_PED` 0x03E3E84F (0 waiting, 1 performing, 2 dormant, 7 not found,
  8 finished). The AI accepts the task; the rope lands far more often when the thrower blocks non-temporary
  events, the target stands still (`TASK_STAND_STILL` `0x919BE13EED931959`) and the target's react task
  (flag 4) is off. `IS_PED_LASSOED` `0x9682F850056C9ADE` signals the catch.
- Optional hogtie: `TASK_HOGTIE_TARGET_PED(Ped, Ped)` `0x27829AFD3E03AC1A` (`SCRIPT_TASK_HOGTIE_TARGET_PED`
  0x5EAF7691), `IS_PED_HOGTIED` `0x3AA24CCC0D451379`, `IS_PED_BEING_HOGTIED` `0xD453BB601D4A606E`,
  `_SET_HOGTIE_ESCAPE_TIMER(Ped, float)` `0xAB591AE6B48B913E` (-1 = never). The AI rarely completes it.
- `TASK_GO_TO_ENTITY(Ped, Entity, int duration, float distance, float speed, float, int)` `0x6A071245EB0D1882`
  brings the thrower into range first.

## HUD

- `_DISPLAY_TEXT` `0xD79334A4BB99BAD1` and `_SET_TEXT_COLOR` `0x50A41AD966910F03` are listed as no-ops since
  build 1436, but older mods still draw visible text with them (ScriptHook translates hashes). Default here:
  `UIDEBUG::_BG_DISPLAY_TEXT` `0x16794E044C9EFB58`, `_BG_SET_TEXT_SCALE` `0xA1253A3C870B6843`,
  `_BG_SET_TEXT_COLOR` `0x16FA5CE47F184F1E`; text always goes through `VAR_STRING(10, "LITERAL_STRING", text)`
  `0xFA925AC00EB830B9`.
- `GRAPHICS::DRAW_RECT(x, y, w, h, r, g, b, a, 0, 0)` `0x405224591DF02025` (x/y = centre).
- `PLAY_SOUND_FRONTEND("NAV_RIGHT", "HUD_SHOP_SOUNDSET", 1, 0)` `0x67C540AA08E4A6F5` as the menu click.

## Misc

- Keys are polled with `GetAsyncKeyState`, not through the ScriptHook keyboard hook.
- `IS_PLAYER_CONTROL_ON` `0x7964097FCE4C244B` = false in cutscenes: leave the NPCs alone then.
- `NETWORK_IS_SESSION_STARTED` `0x9DE624D2FC4B603F`: never active in Red Dead Online.
- `near` and `far` are macros in windows.h; never use them as identifiers.
