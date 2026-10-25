SHROUDMENU 1.1.0 - Red Dead Redemption 2 (Story Mode)
by shrouddeath

A small in-game menu that changes how the people around you behave: make
everyone drunk, have them insult you, attack you or run away screaming, let
them rob and lasso each other, and drop anyone you hit to the ground (they
always get up again). It never touches you, your gang or anyone a mission is
using, so the story keeps working.

STORY MODE ONLY. Never start Red Dead Online with script mods installed.

----------------------------------------------------------------------------
REQUIREMENTS
----------------------------------------------------------------------------
- ScriptHookRDR2 by Alexander Blade (version matching your game)
  http://www.dev-c.com/rdr2/scripthookrdr2/

----------------------------------------------------------------------------
INSTALLATION
----------------------------------------------------------------------------
Game folder = the folder that contains RDR2.exe

1. Copy ShroudMenu.asi and ShroudMenu.ini into the game folder.
2. Start the game in story mode. ShroudMenu.log appears in the game folder
   and a small line in the lower left tells you the menu key.

Updating from 1.0.0: replace ShroudMenu.asi. Replace ShroudMenu.ini as well,
or copy the [Robbery] and [Lasso] sections from the new ini into yours.

----------------------------------------------------------------------------
USAGE
----------------------------------------------------------------------------
  F6         open / close the menu
  Numpad 8/2 move      Numpad 4/6 change a value
  Numpad 5   select    Numpad 0   back
  (arrow keys, Enter and Backspace work as well)

NPC mode (one at a time, everyone within the radius around you):
  Off         nothing
  Drunk       staggering, drunk babbling, the occasional fall
  Insulting   people who see you insult you, curse at you, tell you to get lost
  Aggressive  everyone attacks you (brawling; "Give weapons" arms them)
  Panic       everyone screams and runs from you
  Robbery     the people around you rob each other: a gun is drawn, hands go
              up, money changes hands, the victim runs
  Lasso       everyone carries a lasso and ropes the next person, holds the
              catch a few seconds and lets go

Glass jaw (separate switch, works with every mode):
  whoever you hit drops like a sack, gets up again and never dies from it.
  Options: bumping into people is enough / they do not fight back.

Settings: radius (10-150 m), include animals (Panic and glass jaw), exclude
lawmen and guards, drunk level, stumbling, weapons and "disable law" for
Aggressive, bump trigger and no retaliation for glass jaw, overlay on/off.
"Reset all NPCs" undoes everything at once. Settings changed in the menu are
remembered (ShroudMenu.state); the NPC mode always starts as Off.

All details, hotkeys, speech lines and timings are in ShroudMenu.ini,
including the [Robbery] and [Lasso] sections.

----------------------------------------------------------------------------
GOOD TO KNOW
----------------------------------------------------------------------------
- Story protection: the player, your horse, gang members and companions,
  anyone a mission is using, people in wagons and (by default) lawmen,
  soldiers and guards are never affected. During cutscenes the mod waits.
- Everything the mod changes on an NPC is undone when the NPC leaves the
  area, when you change the mode or when you reset.
- While glass jaw is on, NPCs you hit are healed every time - you cannot
  kill anyone with glass jaw switched on.
- Aggressive mode: witnesses still report you if you shoot back. Switch
  "Disable law (Aggressive)" on if you do not want the law to show up.
- Robbery is a crime like any other: bystanders may panic and the law may
  step in. Pairs form on their own; everyone can be robber or victim.
- Lasso: the catch is held on the rope for a few seconds. Hogtying can be
  switched on in the ini (Hogtie=1), but the AI rarely finishes it.
- F5 is used by other popular trainers and F8 by ScriptHookRDR2 .NET, so the
  menu key is F6. Change it under [Keys] in ShroudMenu.ini.
- If the menu stays invisible, set TextBackend=legacy under [UI].

----------------------------------------------------------------------------
UNINSTALL
----------------------------------------------------------------------------
Delete ShroudMenu.asi, ShroudMenu.ini, ShroudMenu.state and ShroudMenu.log.

----------------------------------------------------------------------------
CHANGELOG
----------------------------------------------------------------------------
1.1.0  New modes Robbery and Lasso. Shorter labels in the settings menu.
       New [Robbery] and [Lasso] sections in ShroudMenu.ini.
1.0.0  First release.

----------------------------------------------------------------------------
CREDITS
----------------------------------------------------------------------------
- Alexander Blade - ScriptHookRDR2
- alloc8or - RDR3 Native DB
- femga - rdr3_discoveries (relationship groups, speech parameters, lasso
  flags, ped list)
- rdr2mods.com - speech line database and the "Making NPCs speak" thread
Source code: https://github.com/shrouddeath/ShroudMenu
