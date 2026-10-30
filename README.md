# Pumpkin Parade Guardian

Pumpkin Parade Guardian is a GBA homebrew tower defense. The ROM has a 240×160
screen, pixel-art towers and monsters, a scrolling 30×18 map, and original
Game Boy Advance PSG audio. The title screen plays a fast, gloomy harmonic
minor melody; gameplay uses responsive sound effects, including a low noise
shuffle as the cursor crosses the patch. A separate gloomy harmonic-minor
score plays during each night and stops when dawn returns.

The GBA build uses a warm autumn palette for the build phase and a deep
purple-green palette at night. Its compact 3×5 pixel lettering has added
character spacing for readability, and the renderer uses paired pixel writes
to keep the screen and controls responsive on original GBA hardware. Game
state is placed in internal work RAM so it also initializes correctly when
an emulator skips the GBA BIOS.

## Build

Install the GNU Arm Embedded Toolchain, then run in PowerShell:

```powershell
.\build.ps1
```

The finished ROM is `build\pumpkin-parade.gba`. It can be opened in a GBA
emulator such as mGBA. This homebrew image leaves the BIOS boot-logo area blank,
so launch it with an emulator's skip-BIOS/fast-boot option; it will not boot
directly through the stock GBA BIOS on original hardware. In Webmulator, set
`mgba use bios` to `OFF` and `mgba skip bios` to `ON` before loading the ROM.

## Controls

- D-pad: move the cursor and pan the map
- A: plant the selected tower or upgrade the tower under the cursor
- B: sell the tower under the cursor
- Select: cycle tower types; over a tower, select its evolution branch (plays a short cue)
- Start: begin the night or pause/resume the parade (the R key has no game action)

The top build HUD previews the selected tower's purchase cost and
damage/speed/effect. Selecting a branch on a placed tower previews its evolved
form and next upgrade cost in the bottom HUD; upgrade it to level 3 with A to
evolve. The top HUD then shows the evolved tower name and live stats while the
cursor is over it. At night, moving over a tower briefly shows its name and
stats in the top HUD before the pan/pause hint returns. Moving to an empty tile
during the day returns the top HUD to the selected build tower.
Level upgrades and evolution branches have distinct outline/color markers.

Defend the patch for five nights. Candy corn pays for towers and upgrades;
moon seeds from surviving pumpkins pay for final upgrades. Lanterns and
cauldrons can harm ghosts, while physical towers cannot. Ghosts first appear
on Night 3, giving the player time to afford magic defenses. Towers fire visible
shots and enemies show their remaining health; the faster night parade ends
at dawn shortly after the final enemy is defeated or escapes.
The first night's zombies and trick-or-treaters have reduced health. The
bottom-right readout uses `D` for damage and `S` for attack speed, followed by
the effect (for example `D20/S2/SWAT`). Selecting an evolution branch shows
the evolved tower name, upgrade cost, and projected level-3 stats in the
bottom HUD. Moving the cursor away returns to the normal build prompt and
contextual cost/stats.
