# Native-Recherche (Stand 25.10.2026)

Quellen:
- [RDR3 NativeDB](https://alloc8or.re/rdr3/nativedb/) (Rohdaten `alloc8or/rdr3-nativedb-data`, lokal durchsucht)
- [femga/rdr3_discoveries](https://github.com/femga/rdr3_discoveries): `AI/RELATIONSHIP`, `AI/FLEE_ATTRIBUTES`,
  `AI/COMBAT_ATTRIBUTES`, `AI/CPED_CONFIG_FLAGS`, `audio/audio_banks`, `peds/peds_list.lua`, `weapons/weapons.lua`
- [rdr2mods.com Speech-Line-DB](https://www.rdr2mods.com/wiki/speechlines/) und der Thread
  [Making NPCs speak (solved)](https://www.rdr2mods.com/forums/topic/467-making-npcs-speak-solved/)
- [alloc8or eCombatAttribute](https://alloc8or.re/rdr3/doc/enums/eCombatAttribute.txt)
- Spiel: Build 1491.17, ScriptHookRDR2 vom 9.2.2023

## Kern: welche Peds dürfen angefasst werden

| Prüfung | Native | Hash |
|---|---|---|
| alle Peds | `worldGetAllPeds` (SDK) | – |
| Spieler | `PLAYER_PED_ID`, `IS_PED_A_PLAYER` | `0x096275889B8E0EE0`, `0x12534C348C6CB68B` |
| Reittier des Spielers | `IS_PED_ON_MOUNT`, `GET_MOUNT` | `0x460BC76A0E10655E`, `0xE7E11B8DCBED1058` |
| tot / sterbend | `IS_ENTITY_DEAD`, `IS_PED_DEAD_OR_DYING` | `0x7D5B1F88E7504BBA`, `0x3317DEDB88C95038` |
| Mensch | `IS_PED_HUMAN` | `0xB980061DA992779D` |
| **Story-Schutz** | `IS_ENTITY_A_MISSION_ENTITY` | `0x138190F64DB4BBD1` |
| Fahrzeug | `IS_PED_IN_ANY_VEHICLE` | `0x997ABD671D25CA0B` |
| Spielergruppe | `GET_PLAYER_GROUP`, `IS_PED_GROUP_MEMBER` | `0x9BAB31815159ABCF`, `0x9BB01E3834671191` |
| Default-Relationship-Group | `GET_PED_RELATIONSHIP_GROUP_DEFAULT_HASH` | `0x42FDD0F017B1E38E` |

Geschützte Gruppen (joaat der Namen aus femga): `PLAYER`, `REL_GANG_DUTCHS`, `REL_GANG_DUTCHS_HORSES`,
`REL_PLAYER_ALLY`, `REL_COMPANION_GROUP`, `REL_PLAYER_LIKE`.
Gesetz/Wachen: `COP`, `REL_COP`, `REL_PLAYER_COP`, `REL_PINKERTONS`, `REL_GUAMA_LAW`, `REL_BOUNTY_HUNTER`,
`REL_GUARD*`, `REL_GREYS_GUARD`, `REL_BRAITH_GUARD` **plus** Modell-Liste, denn ambiente Gesetzeshüter
(`s_m_m_ambientlawrural_01`, `u_m_m_valsheriff_01`, …) stehen laut femga in `REL_CIVMALE`.

## Relationship-Typen (RDR3, nicht wie GTA V!)

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

`SET_RELATIONSHIP_BETWEEN_GROUPS(6, hostile, player)` in beide Richtungen; `ADD_RELATIONSHIP_GROUP`
(`0xF372BC22FCB88606`) liefert den Hash per Out-Parameter.

## Speech

`AUDIO::PLAY_PED_AMBIENT_SPEECH_NATIVE(Ped, ScriptedSpeechParams*)` `0x8E04FEDD28D42462` (alter Name
`_PLAY_AMBIENT_SPEECH1`; die SDK-natives.h von 2019 hat eine falsche 2-Parameter-Signatur).

```cpp
struct ScriptedSpeechParams {          // 0x40 Bytes
    const char* speechName;            // z. B. "GENERIC_INSULT_HIGH"
    const char* voiceName;             // nullptr = eigene Stimme (LMS-Test) oder Voice-Bank-Name
    alignas(8) int variation;          // 0 = zufällige Zeile des Kontexts
    alignas(8) Hash speechParamHash;   // Offset 24
    alignas(8) Ped listenerPed;
    alignas(8) BOOL syncOverNetwork;   // 1
    alignas(8) int v7, v8;             // 1, 1 (Mission-Skripte setzen f_5/f_6 = 1)
};
```

Speech-Params (joaat von `SPEECH_PARAMS_<NAME>`, Tabelle aus femga `audio_banks`):
`force` 0xF85DD899 · `force_shouted_critical` 0xFBDA0604 (R* nutzt es für PLEAD) · `beat_shouted_clear`
0x67F3AB43 (NativeDB-Beispiel) · `force_shouted_clear` 0x9DE2F602 · `standard` 0x3CA9FB81 ·
`shouted` 0xC3159875 · `force_normal` 0x5BDE1482 · `force_normal_clear` 0xA7E68047.
0xE7176FB2 (funktionierte im LMS-Test) passt zu keinem bekannten Namen → in der ini als `lms`.

Speech-Lines, in der rdr2mods-DB bestätigt (Anzahl Stimmen): `GENERIC_INSULT_HIGH` (50), `GENERIC_INSULT_MED`
(50), `GENERIC_CURSE_HIGH` (700), `GENERIC_CURSE_MED` (700), `GET_LOST` (143), `WHATS_YOUR_PROBLEM` (717),
`CHALLENGE_THREATEN` (617), `CIVILIAN_TAUNT_MED` (138), `PANIC_HELP` (490), `SCARED_HELP` (473),
`GENERIC_FRIGHTENED_HIGH` (694), `GENERIC_SHOCKED_HIGH` (737), `PLEAD` (710), `DRUNK` (366).
Nur in der Kontextliste (Pastebin), nicht einzeln geprüft: `GENERIC_ANGRY_SHOUT`, `DISMISSIVE_REACT_ANNOYED`,
`SCREAM_TERROR`, `SCREAM_SHOCKED`, `GUN_RUN`, `CHALLENGE_THREATEN_DRUNK`. **`ANTAGONIZE` existiert nicht.**
`DOES_CONTEXT_EXIST_FOR_THIS_PED` `0x49B99BF3FDA89A7A` prüft vorher, ob die Stimme die Zeile hat.

## Betrunken

- `PED::_SET_PED_DRUNKNESS(Ped, BOOL enabled, float level)` `0x406CCF555B04FAD3` – SOBER 0.0, SLIGHTLY 0.25,
  MODERATELY 0.5, VERY 1.0. Dazu `_IS_PED_DRUNK` `0x50F124E6EF188B22`, `_GET_PED_DRUNKNESS` `0x6FB76442469ABD68`.
- `AUDIO::SET_PED_IS_DRUNK` `0x95D2D383D5396B8A` (Sprachvarianten).
- `PED::_SET_PED_MOTIVATION(Ped, int state, float threshold, Ped target)` `0x06D26A96CA1BCA75`,
  eMotivationState: 0 TOILET, 1 FEAR, 2 ANGRY, 3 AGITATION, …, 10 DRUNK (optional, `UseMotivation=1`).
- Attribut `SA_INEBRIATED` = 10 (`SET_ATTRIBUTE_POINTS` `0x09A59688C26D88DF`) ist der Spieler-Weg; nicht benutzt.
- Fallback: kurzer Ragdoll als Stolperer (`SET_PED_TO_RAGDOLL` `0xAE99FB955581844A`).
- Drunk-Szenarien (femga): `WORLD_HUMAN_DRINKING_DRUNK`, `WORLD_HUMAN_DRUNK_BRACE_WALL`,
  `WORLD_HUMAN_DRUNK_PASSED_OUT_FLOOR` – für später (`TASK_START_SCENARIO_IN_PLACE_HASH` `0x524B54361229154F`).

## Aggressiv

- `TASK_COMBAT_PED(Ped, Ped, int, int)` `0xF166E48407BAC484`, R*-Werte `(…, 0, 16)`.
- `SET_PED_COMBAT_ATTRIBUTES` `0x9F7794730795E019`: 5 ALWAYS_FIGHT, 17 ALWAYS_FLEE, 46
  CAN_FIGHT_ARMED_PEDS_WHEN_NOT_ARMED, 58 DISABLE_FLEE_FROM_COMBAT (alloc8or eCombatAttribute).
- `SET_PED_COMBAT_MOVEMENT` 2 = offensiv, `SET_PED_COMBAT_ABILITY` 0/1/2, `SET_PED_COMBAT_RANGE` 0 = nah,
  `SET_PED_ACCURACY` 0–100.
- `GIVE_WEAPON_TO_PED(Ped, Hash, int ammo, BOOL inHand, BOOL inHolster, int attachPoint, BOOL multiple,
  float 0.5, float 1.0, Hash addReason, BOOL ignoreUnlocks, float degradation, BOOL)` `0x5E3BDDBCB83F3D84`,
  `addReason = joaat("ADD_REASON_DEFAULT") = 0x2CD419DC`. Waffen: `WEAPON_MELEE_KNIFE` 0xDB21AC8C,
  `WEAPON_REVOLVER_CATTLEMAN` 0x169F59F7 (femga weapons.lua).
- `LAW::_SET_LAW_DISABLED(BOOL)` `0x8DE82BC774F3B862` für „Disable law“.

## Glaskinn

`HAS_ENTITY_BEEN_DAMAGED_BY_ENTITY(e1, e2, 1, 1)` `0x7B6E7BEC1143AC86` → `CLEAR_ENTITY_LAST_DAMAGE_ENTITY`
`0xBB19AC7D4DCEFD0F` → `SET_PED_CAN_RAGDOLL` `0xB128377056A54E2A` → `SET_PED_TO_RAGDOLL(ped, min, max, type, 0, 0, NULL)`
→ `APPLY_FORCE_TO_ENTITY` `0xF15E8F5D333F09C4` → `SET_ENTITY_HEALTH(ped, GET_ENTITY_MAX_HEALTH(ped, 0), 0)`
(`0xAC2767ED8BDFAB15`, `0x15D757606D170C3C`). Anrempeln: `IS_ENTITY_TOUCHING_ENTITY` `0x9A2304A64C3C8423`.
Nicht zurückschlagen: `SET_BLOCKING_OF_NON_TEMPORARY_EVENTS` `0x9F8AA94D6D97DBF4`.
Weitere Kandidaten: `SET_PED_RAGDOLL_ON_COLLISION` `0xF0A4F1BBF4FA7497`, `_SET_PED_KNOCKED_BY_ONE_HIT`
`0x5BCF0B79D4F5DBA3`, Config-Flag 29 `PCF_DisableMeleeHitReactions`.

## Panik

`TASK_SMART_FLEE_PED(Ped, Ped from, float distance, int time, int fleeType, float speed, Ped)` `0x22B0D0E37CCB840D`
(time -1 = endlos, speed 3.0 wie R*), `SET_PED_KEEP_TASK` `0x971D38760FBC02EF`, `IS_PED_FLEEING` `0xBBCCE00B381F8482`.
Flee-Attribute (femga): 65536 FORCE_EXIT_VEHICLE, 1048576 DISABLE_MOUNT_USAGE, 4194304 DISABLE_ENTER_VEHICLES.

## HUD

- `_DISPLAY_TEXT` `0xD79334A4BB99BAD1` und `_SET_TEXT_COLOR` `0x50A41AD966910F03` sind laut NativeDB seit
  Build 1436 Nullsubs – ältere Mods zeichnen damit aber weiterhin sichtbar Text (ScriptHook übersetzt Hashes). Standard hier:
  `UIDEBUG::_BG_DISPLAY_TEXT` `0x16794E044C9EFB58`, `_BG_SET_TEXT_SCALE` `0xA1253A3C870B6843`,
  `_BG_SET_TEXT_COLOR` `0x16FA5CE47F184F1E`; Text immer über `VAR_STRING(10, "LITERAL_STRING", text)` `0xFA925AC00EB830B9`.
- `GRAPHICS::DRAW_RECT(x, y, w, h, r, g, b, a, 0, 0)` `0x405224591DF02025` (x/y = Mitte).
- `PLAY_SOUND_FRONTEND("NAV_RIGHT", "HUD_SHOP_SOUNDSET", 1, 0)` `0x67C540AA08E4A6F5` als Menü-Klick.

## Sonstiges

- Tasten per `GetAsyncKeyState` gepollt (wie Infinite Hair), nicht über den ScriptHook-Keyboard-Hook.
- `IS_PLAYER_CONTROL_ON` `0x7964097FCE4C244B` = false in Cutscenes → NPCs in Ruhe lassen.
- `NETWORK_IS_SESSION_STARTED` `0x9DE624D2FC4B603F` → in RDO nie aktiv werden.
