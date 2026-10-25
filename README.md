# ShroudMenu - RDR2 (Story Mode)

A small in-game menu that changes how the people around you behave: make everyone
drunk, have them insult you, attack you or run away screaming, and drop anyone you
hit to the ground. It never touches you, your gang or anyone a mission is using, so
the story keeps working.

**Download for players:** Nexus Mods (release zip). This repository holds the source code.

> Story mode only. Never start Red Dead Online with script mods installed.

## Features

Open the menu with **F6**. Numpad 8/2 move, 4/6 change a value, 5 select, 0 back
(arrow keys, Enter and Backspace work as well). One NPC mode is active at a time and
applies to everyone within the radius around you:

| Mode | What happens |
|---|---|
| Off | nothing |
| Drunk | staggering, drunk babbling, the occasional fall |
| Insulting | people who see you insult you, curse at you or tell you to get lost |
| Aggressive | everyone attacks you (brawling; optionally armed) |
| Panic | everyone screams and runs from you |

**Glass jaw** is a separate switch that stacks on any mode: whoever you hit drops to
the ground like a sack, gets up again and never dies from it. Options: bumping into
people is enough, and they do not fight back.

**Settings:** radius (10-150 m), include animals (Panic and glass jaw), exclude lawmen
and guards, drunk level, stumbling, weapons and "disable law" for Aggressive, bump
trigger and no retaliation for glass jaw, overlay on/off. **Reset all NPCs** undoes
everything at once. Menu settings are remembered in `ShroudMenu.state`; the NPC mode
always starts as Off.

## Story protection

Every scan the mod collects the peds around the player and skips the player, the
player's horse, gang members and companions (relationship groups and the player
group), mission entities, people in vehicles and, by default, lawmen, soldiers and
guards (by relationship group and by model). Nothing happens during cutscenes,
loading screens or in Red Dead Online. Everything the mod changes on an NPC is
tracked and undone when the NPC leaves the area, the mode changes or the player resets.

## Install

1. ScriptHookRDR2 by Alexander Blade (`dinput8.dll`, `ScriptHookRDR2.dll`) in the game folder.
2. Copy `ShroudMenu.asi` and `ShroudMenu.ini` next to `RDR2.exe`.
3. `ShroudMenu.ini` has every hotkey, timing and speech line with comments.
   `ShroudMenu.log` in the game folder tells you what the mod did.

Good to know: with glass jaw on, hit NPCs are healed every time, so they cannot be
killed. In Aggressive mode witnesses still report you unless "Disable law" is on.
F5 belongs to popular trainers and F8 to ScriptHookRDR2 .NET, hence F6.

## Build

Visual Studio 2022 with C++ (x64) and the ScriptHookRDR2 SDK unpacked to `sdk\`
(`sdk\inc\main.h`, `sdk\lib\ScriptHookRDR2.lib`).

- `build.bat` produces `build\ShroudMenu.asi`
- `test.bat` runs the offline unit test (hashes, ini parsing, speech struct layout)
- `python tools\make_release.py` zips the release

```
src/
  script.cpp    main loop, menu wiring, overlay
  modes.cpp     the modes (Drunk, Insulting, Aggressive, Panic) and glass jaw
  scanner.cpp   which NPCs may be touched (story protection, lawmen, radius)
  speech.cpp    PLAY_PED_AMBIENT_SPEECH_NATIVE with the ScriptedSpeechParams struct
  ui.cpp        menu drawing and navigation
  config.cpp    ini/state parsing, joaat, key names
  natives.h     natives called by hash (RDR3 NativeDB)
```

Research notes on the natives used (German): [docs/NATIVES.md](docs/NATIVES.md).

## Credits

- Alexander Blade - ScriptHookRDR2
- alloc8or - RDR3 Native DB
- femga - rdr3_discoveries (relationship groups, speech parameters, ped list)
- rdr2mods.com - speech line database and the "Making NPCs speak" thread

## License

MIT, see [LICENSE](LICENSE). Covers the source code only; the game and its assets
belong to Rockstar Games, ScriptHookRDR2 to Alexander Blade.
