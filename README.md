# Pumpkin Parade Guardian

Pumpkin Parade Guardian is a GBA homebrew tower defense. The ROM has a 240×160
screen, pixel-art towers and monsters, a scrolling 30×18 map, and original
Game Boy Advance PSG audio. The title screen plays a fast, gloomy harmonic
minor melody; gameplay uses responsive sound effects, including a low noise
shuffle as the cursor crosses the patch. A faster, separate gloomy
harmonic-minor score plays during each night and stops at dawn, followed by a
brief happy major-key dawn cue.

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
form and next upgrade cost in the bottom HUD; press A once to upgrade a tower
to level 2 and evolve it, then again to buy its level-3 power upgrade. The top
HUD then shows the evolved tower name and live stats while the
cursor is over it. At night, moving over a tower briefly shows its name and
stats in the top HUD; moving to an empty tile immediately restores the
pan/pause hint. At dawn the display and cursor jump to the topmost, leftmost
built tower. Moving to an empty tile during the day returns the top HUD to the
selected build tower.
Level upgrades and evolution branches have distinct outline/color markers.

## Tower reference

Tower stats use `D/S/effect`: damage per target, the compact attack-speed
rating shown in the HUD, and a short effect label. A dash means no special
effect. Ranges are Manhattan-distance pixels; all ordinary towers gain 7
pixels of range per upgrade, except Straw Sentinel, whose short reach is part
of its melee role. Splash towers divide their listed damage among nearby
targets as noted below. A `Moonflower` does not attack; its `D0/S0` stats
describe its support-only role. Level 2 is the selected evolution; level 3
strengthens it further.

| Tower | Buy | Level 1 | Level 2 evolution | Level 3 | Range L1 / L2 / L3 |
|---|---:|---|---|---|---:|
| Scarecrow | 50 | D9/S4/- | Crow Caller: D6/S7/flock | D8/S7/flock | 56 / 63 / 70 |
| Scarecrow | 50 | D9/S4/- | Straw Sentinel: D24/S4/swat | D32/S4/swat | 56 / 22 / 22 |
| Jack-o'-Lantern | 75 | D20/S2/burn | Inferno Jack: D28/S2/splash | D36/S3/splash | 44 / 51 / 58 |
| Jack-o'-Lantern | 75 | D20/S2/burn | Will-o'-Wisp: D42/S2/poison | D54/S3/poison | 44 / 51 / 58 |
| Witch's Cauldron | 65 | D5/S2/slow | Candy Glue: D7/S2/glue | D9/S2/glue | 49 / 56 / 63 |
| Witch's Cauldron | 65 | D5/S2/slow | Hex Pot: D7/S2/hex | D9/S2/hex | 49 / 56 / 63 |
| Moonflower | 80 | D0/S0/buff | Blood Moon: D0/S0/fear | D0/S0/fear | 52 / 59 / 66 |
| Moonflower | 80 | D0/S0/buff | Star Bloom: D0/S0/buff | D0/S0/buff | 52 / 59 / 66 |
| Bat Roost | 90 | D7/S6/swarm | Night Swarm: D9/S6/swarm | D12/S8/swarm | 68 / 75 / 82 |
| Bat Roost | 90 | D7/S6/swarm | Vampire Bats: D18/S6/drain | D24/S8/drain | 68 / 75 / 82 |

Upgrade cost is 60 candy for level 2. Level 3 costs 85 candy and 2 moon seeds.
Scarecrow branches have distinct combat behavior: Crow Caller attacks more
quickly and shares its damage among nearby enemies; Straw Sentinel hits
twice as hard, but has a 22-pixel range and a slower attack. Inferno Jack
splash shots deal half damage to nearby enemies; Will-o'-Wisp adds poison.
Candy Glue applies a longer slow; Hex Pot spreads poison. Blood Moon slows
nearby enemies while Star Bloom strengthens nearby tower cooldown buffs.
Night Swarm attacks nearby enemies for half damage; Vampire Bats double their
damage. Level-2/3 projectiles use branch-specific colors, making evolved
attacks distinguishable from base tower shots.

## Monster reference

Monster speed is the movement value used by the game (higher values move
farther per update). Health scales with the current night as listed; rewards
are candy corn per defeat.

| Monster | Appears | Health | Speed | Reward | Notes |
|---|---:|---:|---:|---:|---|
| Zombie | Night 1 | 30 + 8 per night | 130 | 11 | Slow, standard enemy |
| Trick-or-Treater | Night 1 | 12 + 4 per night | 300 | 5 | Fast, fragile |
| Skeleton | Night 2 | 22 + 6 per night | 285 | 8 | Fast; reduced early-wave health |
| Ghost | Night 3 | 32 + 9 per night | 220 | 9 | Only Lantern and Cauldron attacks can hurt it |
| Werewolf | Night 3 | 95 + 18 per night | 175 | 17 | Heavy enemy |
| Headless Horseman | Night 5 boss | 450 | 125 | 45 | Final enemy of Night 5 |

Defend the patch for five nights. Candy corn pays for towers and upgrades;
moon seeds from surviving pumpkins pay for final upgrades. Lanterns and
cauldrons can harm ghosts, while physical towers cannot. Ghosts first appear
on Night 3, giving the player time to afford magic defenses. Towers fire visible
shots and enemies show their remaining health; the faster night parade ends
at dawn shortly after the final enemy is defeated or escapes.
The first night's zombies and trick-or-treaters have reduced health, and
Night 2 skeletons have reduced health. The bottom-right readout uses `D` for
damage and `S` for attack speed, followed by
the effect (for example `D20/S2/SWAT`). Selecting an evolution branch shows
the evolved tower name, upgrade cost, and projected level-3 stats in the
bottom HUD. Crow Caller favors faster flock attacks while Straw Sentinel
trades attack speed and range for heavier swats; their sprite colors/shapes
and displayed damage/speed now differ. Moving the cursor away returns to the
normal build prompt and contextual cost/stats.
