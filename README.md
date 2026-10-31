When the sun goes down, they come for the pumpkins. Pumpkin Parade Guardian is a cozy-spooky tower defense for Game Boy Advance: spend the calm daylight hours planting scarecrows, lanterns, cauldrons, and stranger defenses around your patch, then hold the line as zombies, ghosts, banshees, and worse parade down the path at night. Survive ten nights, grow your towers into wild evolutions, and face the Headless Horseman himself on the final night — every pumpkin you save is a victory.

## Fix 5 — Fix4 broken on phone: BIOS IRQ-return bug (approved Oct 31, 2026; shipped as release 1.1)

Fix 4 verdict (Cori, Oct 31, 2026): "This one loads but the title
screen is a blue bar at the top and the rest all black buttons do
nothing so game is broke" — Fix4 is superseded and must not be
re-delivered. The symptom fits a hang mid-way through the first
title draw: the first title frame draws directly to the displayed
VRAM page top-to-bottom, the first VBlank IRQ fires ~16 ms after
start, and the main loop never progressed past it.

Root cause, proven from the binary (not guessed): Fix4's handler was
declared __attribute__((interrupt("IRQ"))). objdump of Fix4's ELF
shows irq_handler starting with `sub lr, lr, #4` and returning via
`ldm sp!, {r0-r7, ip, pc}^` — a raw IRQ return that restores CPSR
from SPSR. That is wrong here: the GBA BIOS IRQ dispatcher calls the
user handler at 0x03007FFC as an ORDINARY subroutine in system mode
and performs the IRQ return itself, so the handler must return with
a plain `bx lr`. In Fix4, lr held the BIOS return address, not an
IRQ return address, so subtracting 4 and returning through SPSR
jumped to the wrong place and hung the game on the first vblank.
Fix4's desktop test missed it because it only checked "no crash
dialog / no Jumped to invalid address" — a hang produces no crash
dialog either, so that was never proof the game runs. Fix5's desktop
test below therefore checks actual progression (PC / VRAM), not just
absence of a crash dialog.

Fix, in main.c only: irq_handler is now a normal C function (the
interrupt("IRQ") attribute is removed entirely); the handler body is
identical — check REG_IF & 0x0001, clear it, count vblanks, call
music_step() exactly once per 5 vblanks, skip when music_hold is
set. vblank_step_count is now volatile (it is written in IRQ
context and reset in main-loop code; Fix4 left it non-volatile).
DISPSTAT=0x0008, IE=0x0001, IME install order, IRQ_VECTOR install,
music_hold guards, and tempo are all unchanged from Fix4: title/night
note = 4 steps x 5 vblanks = 20 vblanks = ~335 ms, dawn note = 3 x 5
= 15 vblanks = ~251 ms, dawn silence = 6 x 5 = 30 vblanks = ~502 ms,
knob MUSIC_VBLANKS_PER_STEP=5. No melody, bass, play_tone envelope,
render, logic, control, or graphics change; no Fix1/Fix2 render
change reintroduced; no revert to the Fix3 timer driver; startup.s
stack setup unchanged (BIOS dispatch runs the handler on the main
stack, SP 0x03007F00, not the IRQ stack).

Desktop proof (Fix5, verified in mGBA-Qt 0.10.5 via its GDB stub,
RSP single-stepping — NOT "no crash dialog"): objdump shows every
Fix5 irq_handler return is `bx lr` (bxeq/bxls/bxne/bx at 0x129c,
0x12c0, 0x12d8, 0x13dc; no `sub lr, #4`, no `ldm {..., pc}^`), while
Fix4 starts with `sub lr, lr, #4` and returns via `ldm {..., pc}^`.
Stepping the first VBlank IRQ in Fix5: handler 0x1290 -> BIOS 0x194
-> BIOS 0x198 -> interrupted ROM code 0x08000be8 (rect/render),
frame_count already 1 — a clean single dispatch and return to the
main loop. Stepping Fix4's first IRQ instead: handler -> BIOS
0x190 (exactly 4 bytes early) -> handler entry 0x08000d00 AGAIN
(the instruction at 0x190 re-dispatches) -> a second handler pass
-> BIOS 0x9c/0xa0... walking BIOS vector code in a corrupted
state — that re-entry is the hang behind Cori's blue-bar/black
title. Host frames are byte-identical to Fix4 on all 7 scenes and
controls pass (see Fix 4 QA, unchanged); IRQ tempo simulation gives
exactly 20-vblank title/night onsets (~335 ms), 15-vblank dawn
(~251 ms), 30-vblank dawn silence (~502 ms), paused = silent.
Limitation, stated plainly: mGBA-Qt's GDB server in this VM pauses
at startup under -g and its continue-after-breakpoint never resumes
(it timed out on every second continue, for both Fix4 and Fix5, even
after stepping away from the breakpoint), so a free-run VRAM dump
of a full title frame after several seconds could NOT be obtained
here — stepping proves the IRQ return chain, it does not prove a
full rendered title on desktop. Cori's RetroArch phone test (full
title, working buttons, evenly spaced night music) remains the
real gate.

## Fix 4 — night music choppiness, VBlank IRQ driver (unpushed, superseded by Fix 5)

Fix 3 verdict: intro music perfect, animation/scrolling back to normal,
but night music was choppy -- notes 3 and 4 seemed to play too quickly
together while other notes had better spacing, and scrolling did not
change it (consistently choppy in every view). Dawn may have had the
same fault, too short to notice.

Diagnosis, with evidence (not guessed): a host simulation of Fix 3's
driver shows it does NOT burst two notes into a single loop at any
plausible loop period -- a note needs 4 steps, a step needs 5 vblanks
of accumulated time, so two notes in one loop would need a 40-frame
loop. What the simulation DOES show is loop-boundary quantization:
Fix 3 emitted steps only at main-loop ends, so step times (and hence
note times) jittered with the loop period: stable loops give an exact
335 ms/note, random 1-3 frame loops give 301-368 ms, and loops with
occasional 5-frame loops give 268-435 ms, all averaging 334 ms -- a
repeating short/long lilt. Night loops vary (tower scans, enemy moves,
spawns, scrolling) while title loops are stable, which matches Cori's
report exactly: intro clean, night choppy, scrolling irrelevant.
Tempo itself was right (Fix 3's ~334 ms) and is unchanged here.

Fix, in main.c only (rules, art, HUD, controls, 10-night/8-tower design
unchanged; still Mode 4, so index.html is unaffected; graphics are
exactly Fix 3's = the original repo render): the melody is now stepped
by a VBlank IRQ, not by the main loop. DISPSTAT bit 3 enables the
VBlank IRQ, IE enables VBlank only, IME is set after the handler
address is installed at 0x03007FFC (gcc interrupt("IRQ"), ARM code),
and the handler acknowledges VBlank in IF, counts vblanks, and calls
music_step exactly once per 5 vblanks -- never more than once per
vblank, never in a burst. The main-loop music_update call and the
Timer0 driver are removed entirely; both drivers never run together.
Tempo is exactly Fix 3's: title/night note = 4 steps x 5 vblanks =
20 vblanks = ~335 ms; dawn note = 3 x 5 = 15 vblanks = ~251 ms; dawn
silence = 6 x 5 = 30 vblanks = ~502 ms; the tuning knob is now
MUSIC_VBLANKS_PER_STEP (5). Race safety: music_step runs in IRQ
context, so stop_music, start_night, and the unpause music_clock reset
are guarded by a music_hold flag the handler checks and skips on
(step counter still resets, so no burst follows a hold); pause still
silences music via music_step's own paused check and resumes without
a burst. Night SFX stay on the separate noise channel.

Verified: diff vs Fix 3 main.c is ONLY the IRQ driver, the hold
guards, the main-loop call removal, and this README section -- render,
key_update (except the hold wrapper on the unpause assignment),
update_game, melodies, and play_tone envelope are unchanged; host
frames are 0 bytes different from Fix 3 on all 7 scenes (title, day
build, crowded night, scrolled-away night, pause, win, lose);
controls pass including two quick Right presses moving the cursor
two cells (5 -> 6 -> 7); IRQ-driver simulation gives exactly
335 ms/note (night/title) and 251 ms/note (dawn) at any loop period.
Desktop mGBA (extracted AppImage binary under Xvfb) ran the ROM for a full 20-second timeout with no crash dialog and no "Jumped to invalid address" -- only expected ALSA no-sound-card warnings. Boot signal only: the OpenGL viewport is not capturable in this environment, so this does not prove audio.

Not proven: RetroArch night-music smoothness by ear on Cori's phone
(the real gate -- Fix 2's lag and Fix 3's choppiness only showed
there), real-device audio, Nights 6-10 balance, and the browser
wrapper (index.html untouched, not rebuilt).

## Fix 3 -- original graphics restored, music tempo slowed further (unpushed, awaiting playtest)

Cori's Fix2 playtest verdict, followed literally: Fix2's animation/graphics were
MORE laggy than the original repo version (cursor movement slower, and two quick
Right presses sometimes missed the second press), while Fix2's music tempo WAS
consistent across all views but still faster than the original tracks. Fix3 therefore
reverts every graphics/render change and keeps only the consistent music driver.

Fix, in main.c only: start from the original repo code (commit 7ae11be) and change
ONLY the music driver. Reverted in full: no static EWRAM backbuffer, no second work
buffer, no DMA3 copy, no static-buffer shift/strip redraw, no projectile-division
precomputation. Render, key_update, update_game (called twice per loop), and the
main loop are byte-identical to the original: every frame draws directly to the hidden
VRAM Mode 4 page (tiles + towers + enemies + projectiles + HUD + message), waits for
vblank, and flips the page. The diff against the original main.c is only: Timer0
registers, MUSIC_TICKS_PER_FRAME (1097), the music_step/music_update driver, timer
fields reset in stop_music, and Timer0 start in main -- verified by git diff (57
insertions, 1 renamed line, no render/logic change). Host-harness frames are
pixel-identical to the original for title, day build, crowded night, scrolled-away
night, pause, win, and lose (0 differing bytes on all 7 scenes), and cursor controls
match the original exactly, including two quick Right presses moving the cursor
two cells (5 -> 6 -> 7 in the harness).

Tempo: the timer-driven driver is kept (Timer0 at 65536 Hz, 1097 ticks per GBA frame,
remainder carried, steps capped at 4 per loop so a stall cannot burst notes, pause
discards elapsed time and resumes without a burst, night SFX stay on the separate
noise channel, title/night/dawn internal counters unchanged), so tempo stays
view-independent as in Fix2. MUSIC_TICKS_PER_STEP is now 5 (Fix2 used 3, which Cori
said was still too fast; a marginal 4 was not chosen, to avoid another round).
Resulting intervals (1 vblank ~ 16.7 ms): title/night note = 4 steps x 5 ticks =
20 ticks = ~334 ms; dawn note = 3 steps x 5 = 15 ticks = ~250 ms; dawn silence =
6 steps x 5 = 30 ticks = ~501 ms. Scheduler simulation: Fix3 holds ~331-335 ms/note
(mean) at loop periods of 1, 2, and 3 frames, vs Fix2's ~201 ms and Fix1's ~67 ms.
These are a restored-by-ear target for Cori to verify -- no emulator audio capture
was possible here, so the exact original milliseconds were not measured; if it
still feels off, the tuning knob is the one constant MUSIC_TICKS_PER_STEP.

Not proven: RetroArch tempo by ear and cursor fluidity on Cori's phone (the real
gate, since Fix2's lag only showed there), real-device audio, Nights 6-10 balance,
and the browser wrapper (index.html untouched, not rebuilt). Desktop mGBA booted
the Fix3 ROM for a full 20-second run with no crash dialog or invalid jump
(boot signal only; the OpenGL viewport is not capturable in this environment).

# Pumpkin Parade Guardian

![screenshot](images/pumpkin-parade-0.png) ![screenshot](images/pumpkin-parade-1.png)

By day, the patch is yours: plant towers, plan your defenses, and spend your candy corn and moon seeds wisely. By night, the parade marches in — zombies and trick-or-treaters first, then ghosts, banshees, moths, and broods that split when they fall. Warm autumn days give way to deep purple-green nights, with a gloomy Halloween score that keeps perfect time no matter how crowded the parade gets. Hold the patch for ten nights, and the Headless Horseman himself leads the final parade.

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
The linker keeps startup constants near the reset stub so BIOS-skipped
emulators can read them during early initialization.

## Controls

- D-pad: move the cursor and pan the map
- A (Z on Webmulator): plant the selected tower or buy its previewed upgrade
- B (X on Webmulator): sell the tower under the cursor
- Select (Shift on Webmulator): cycle tower types; over a tower, preview its
  evolution or next upgrade
- Start (Enter on Webmulator): begin the night or pause/resume the parade
- Title screen Left/Right: choose difficulty (Normal starts with 12 pumpkins;
  Hard starts with 8)
- Select previews only; it never changes tower stats or spends candy

The top build HUD previews the selected tower's purchase cost and
damage/speed/effect. Select previews a level-1 tower's evolution branch, or a
level-2 tower's level-3 upgrade, in the bottom HUD; it never changes the tower
or spends resources. Press A to buy the previewed level-2 evolution or upgrade.
The top HUD then shows the evolved tower name and live stats while the cursor
is over it. At night, moving over a tower briefly shows its name and stats in
the top HUD before the pan/pause hint returns. Moving to an empty tile during
the day returns the top HUD to the selected build tower. During the night,
moving the cursor over a monster briefly shows its name, current/maximum HP,
movement speed, and active slow/poison effects in the bottom HUD.
Level upgrades and evolution branches have distinct color markers.
Pausing the parade also silences its music; the night theme resumes when play
resumes. Nighttime sound effects use a separate noise channel so enemy defeat
and escape sounds cannot interrupt the music melody or bass. Upgraded towers
use branch-specific palette colors and small evolution-themed sprite details
instead of box outlines. Enemy health bars
appear after the enemy takes damage; their scaling is precomputed at spawn to
avoid per-frame division during combat. Night sky star positions are also
precomputed, and off-screen towers are skipped during drawing.

Defend the patch for ten nights. Candy corn pays for towers and upgrades;
moon seeds from surviving pumpkins pay for final upgrades. Lanterns and
cauldrons can harm ghosts, while physical towers cannot. Ghosts first appear
on Night 3, giving the player time to afford magic defenses. Towers fire visible
shots and enemies show their remaining health. Pumpkin Broods, Bramble Brutes,
Banshees, and Moon Moths join the parade from Nights 6-9; Broods split into two
seedlings when defeated. The Headless Horseman leads Night 10. The faster night
parade ends at dawn shortly after the final enemy is defeated or escapes.
The first night's zombies and trick-or-treaters have reduced health. The
bottom-right readout uses `D` for damage and `S` for attack speed, followed by
the effect (for example `D20/S2/SWAT`). Selecting an evolution branch shows
the evolved tower name, upgrade cost, and projected level-2 stats in the
bottom HUD. Moving the cursor away returns to the normal build prompt and
contextual cost/stats.

## Tower stats

Damage is per primary enemy hit; attack speed is approximate attacks per
second at normal game speed. Level 2 costs 60 candy and evolves the selected
branch. Level 3 costs 85 candy and 2 moon seeds and strengthens the evolved
tower. Branch selection previews the level-2 total stats, not an additive
bonus. Attack ranges are Manhattan-distance pixels and have been increased by
25%; Moonflower range is its aura radius in tiles.

| Tower | Cost | Level 1 | Level 2 evolution | Level 3 upgrade | Range L1/L2/L3 |
|---|---:|---|---|---|---:|
| Scarecrow Slinger | 50 | D9/S4/- | Crow Caller: D6/S7/flock | D8/S7/flock | 70/78/87 |
|  |  |  | Straw Sentinel: D24/S4/swat | D32/S4/swat | 70/27/36 |
| Jack-o'-Lantern | 75 | D20/S2/burn | Inferno Jack: D28/S2/splash | D36/S3/splash | 55/63/72 |
|  |  |  | Will-o'-Wisp: D42/S2/poison | D54/S3/poison | 55/63/72 |
| Witch's Cauldron | 65 | D5/S2/slow | Candy Glue: D7/S2/glue | D9/S2/glue | 61/70/78 |
|  |  |  | Hex Pot: D7/S2/hex | D9/S2/hex | 61/70/78 |
| Moonflower | 80 | D0/S0/buff | Blood Moon: D0/S0/fear | D0/S0/fear | 5/6/7 tiles |
|  |  |  | Star Bloom: D0/S0/buff | D0/S0/buff | 5/6/7 tiles |
| Bat Roost | 90 | D7/S6/swarm | Night Swarm: D9/S6/swarm | D12/S8/swarm | 85/93/102 |
|  |  |  | Vampire Bats: D18/S6/power | D24/S8/power | 85/93/102 |
| Thorn Maze | 125 | D14/S1/pierce | Briar Crown: D19/S2/root | D25/S3/root | 77/86/95 |
|  |  |  | Rose Sentinel: D23/S2/root | D30/S3/root | 77/101/110 |
| Grave Bell | 145 | D12/S1/pulse | Tollkeeper: D17/S2/stun | D22/S2/stun | 67/88/97 |
|  |  |  | Soul Choir: D21/S2/stun | D27/S3/stun | 67/76/84 |
| Star Compass | 170 | D30/S1/chain | Comet Seeker: D38/S1/chain | D49/S1/chain | 95/103/112 |
|  |  |  | Orrery: D42/S1/chain | D55/S2/chain | 95/121/130 |

Crow Caller attacks a group with half damage per enemy; Straw Sentinel deals
more damage at a short range. Inferno Jack splashes half damage onto nearby
enemies and poisons them; Will-o'-Wisp poisons its target. The Cauldron's slow
cuts movement speed by 75% for about one second; Candy Glue cuts it by about
83% for about two seconds. Hex Pot spreads poison. Blood Moon slows nearby
enemies and Star Bloom strengthens nearby tower cooldown buffs. Night Swarm
deals half damage to nearby enemies; Vampire Bats double their primary damage.
Evolved towers use larger, branch-colored projectiles. Crow Caller and Straw
Sentinel also have distinct silhouettes and colors, and their combat stats
match the HUD preview.

Evolved towers use larger, branch-colored projectiles. Crow Caller and Straw
Sentinel also have distinct silhouettes and colors. Their damage, range, and
attack speed are calculated separately, so the preview matches combat.

Late-game towers have higher purchase costs; they use the same level-up costs
and evolution selection as the other towers.

Thorn Maze pierces into clustered enemies, damaging nearby monsters for half
damage and hindering them; Briar Crown emphasizes area control, while Rose
Sentinel trades control for greater damage and range. Grave Bell damages and
slows nearby monsters; Tollkeeper projects a wider slowing pulse, while Soul
Choir delivers stronger, faster pulses. Star Compass chains damage to up to
two nearby enemies, dealing reduced damage to chained targets; Comet Seeker
fires chain bolts more rapidly, while Orrery extends the reach of its chain
attacks.

## Monster stats

HP is the value on the first night that monster appears. HP increases by the
listed amount per night through Night 4; from Night 5 onward, each night's HP
increase is 25% larger (rounded down). Pumpkin Brood and Seedling HP use the
same scaling. Speed is an internal movement value (higher is faster). Candy is
the kill reward. Hard mode adds three monsters to each wave and increases
zombie, werewolf, and Horseman HP by 20%; other enemy stats are unchanged.

| Monster | Appears | HP | Speed | Candy | Notes |
|---|---:|---:|---:|---:|---|
| Zombie | Night 1 | 38 (+8/night) | 130 | 11 | Slow and sturdy |
| Trick-or-treater | Night 1 | 16 (+4/night) | 300 | 5 | Fast, fragile swarm |
| Skeleton | Night 2 | 36 (+6/night) | 285 | 8 | Fast, reduced early HP |
| Ghost | Night 3 | 59 (+9/night) | 220 | 9 | Only magic towers can hurt it |
| Werewolf | Night 3 | 149 (+18/night) | 175 | 17 | Tough |
| Pumpkin Brood | Night 6 | 284 | 170 | 18 | Splits into two 86-HP seedlings on defeat |
| Seedling | On Brood defeat | 86 | 310 | 3 | Two per Brood; HP scales with wave |
| Bramble Brute | Night 7 | 340 | 140 | 25 | Heavy, slow bruiser |
| Banshee | Night 8 | 185 | 230 | 18 | Only magic towers can hurt it |
| Moon Moth | Night 9 | 164 | 355 | 14 | Very fast flyer |
| Headless Horseman | Night 10 | 1300 | 135 | 75 | Final boss; 1560 HP in Hard |
