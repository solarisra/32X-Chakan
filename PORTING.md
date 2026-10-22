# SGDK to MarsDev port

The first migration slice keeps gameplay on the Genesis 68000 and Genesis VDP,
while retaining the MarsDev 32X ROM boot structure. The SH-2 master initializes
the 32X in transparent 8-bit mode; Genesis-side rendering remains responsible
for the game image.

## Game states: logo, intro, menu

The normal `GAME=PORTAL_HUB` ROM now boots through `STATE_LOGO` → `STATE_INTRO` → `STATE_MENU`. The menu includes lightning, title reveal, eye cycling,
intro music, Start Game and Options. Start opens the main menu; Left/Right select,
and Start confirms. Options retains the source Back/unused second selection.
Start Game enters `STATE_PLAY`/Portal Hub. Pause is handled inside `STATE_PLAY`; `STATE_PAUSE` remains a reserved source enum value. Standalone stage
builds continue to boot directly into their selected stage for testing. Start
+A+C returns to the Hub during gameplay and does not replay the logo.

State dispatch is `sh_src/inc/gamestate.h` and `sh_src/src/gamestate.c`;
logo files retain `inc/gamestates/` and `src/gamestates/`. Original logo PNGs
keep their source paths under `sh_src/res/`. Rendering uses only the 32X VDP:
the original 96x32 SGDK logo at (114,80), horizontally centered on its 92 visible columns, blue cycling for 123 ticks, a 60-tick
hold, red cycling for 100 ticks, another 60-tick hold, and the source's five-tick
fade plus 25-tick black hold. The scream fires on the sixth red tick. Startup
music stays silent during the logo, then starts with the intro. Logo input is unskippable as in source.

The palette animation keeps the source's every-third-tick cadence and valid
0..10 indices; the original initialized slots to 3..13 despite an 11-color
array. This out-of-bounds read is corrected by clamping the initial phase at the dark endpoint.
The blue sprite's pixels are retained throughout, changing only palette data,
matching the source (the red PNG supplies its palette only).

The original `md_src/res/sfx/chakan_scream2.wav` is converted at build time to
signed 8-bit mono 14kHz PCM, padded to 256 bytes for the current XGM1 driver.
The sample is stretched to play at 75% of its original speed, lowering its pitch.
The 68000 registers it as sample 63 and plays it on PCM channel 0. This retains
the established audio driver and fixed ROM mirror without introducing XGM2.
The logo frame uses its own level-memory overlay, replaced by intro/menu and eventually Hub assets after
the synchronous render barrier. A freestanding `memset` supports frame clearing.

`STATE_INTRO` ports all five story scenes from the original assets. Scene 1
includes palette lightning, six-frame Chakan animation behind the scrolling
text, a 20-tick fade, and the original 5/16-pixel crawl. Scenes 2–4 show the
illustrated story panels. Scene 5 scrolls the final text over the hourglass,
cycling its three sand colors every eleven ticks. All crawls stop within the
visible tilemap bounds. Blocking source fades and cleanup are counted as frame
ticks: 2343, 522, 522, 522, and 1902 ticks for the five scenes.

Start fades the intro to black for five ticks and holds for five more before
entering the menu. A held Start is consumed across the transition. Intro and
menu each start the original intro theme; the opening intro lightning uses its
own PCM recording. Intro scratch data shares the existing level-memory overlay.
The RGB opening background's four palette-header rows are removed, preserving
its original PAL0 artwork and PAL2 lightning indices. Lightning pulses remain
latched across simulation catch-up until `gamestate_frame_presented()` acknowledges
a displayed frame, preventing one-tick flashes from disappearing at 30 fps.

Validation: host tests reconstruct every plane and all six animation frames,
exercise the complete state sequence and menu controls under UB sanitization,
and verify skip fading and crawl bounds. PicoDrive completed the sequence
(logo → intro at VBlank 393, intro → menu at 6210) and a separate Start-skip run
reached the menu at VBlank 708. Opening crawl, story panels, hourglass and menu
captures were inspected. Real-hardware confirmation remains pending.

```sh
python3 -m unittest discover -s sh_src/tools -p test_intro.py
python3 -m unittest discover -s sh_src/tools -p test_logo.py
python3 md_src/tools/check_audio.py --core /path/to/picodrive_libretro.so --intro --frames 7000 --min-render-fps 60
```

## Intro rendering at 60 presentations per second

The intro uses a prepared-screen compositor (`GdlScreen`). `convert_intro.py`
prepares full 320x224 background words, sparse four-pixel text patches, and six
word-aligned rectangles containing Chakan already composed over the background.
Story panels 2–4 have their text composed into the static screen. Scene loading
copies only the current screen, text patches, row directory, and animation into
the intro SDRAM overlay, with shared uploads and the scheduler's cache barrier.
The combined cache is about 191 KiB and does not increase the maximum level
overlay size. It is replaced when gameplay loads.

Both SH-2s retain the equal 112-line split. The hot path copies cached scanlines,
substitutes the current animation rectangle, then applies sparse text patches
using whole-word masks. It rebuilds each backbuffer completely, avoiding stale
pixels when scrolling, animating or changing scenes. Palette lightning and
hourglass animation still work without modifying pixel buffers. The general
renderer remains available for gameplay and as the host pixel reference.

PicoDrive NTSC measurements in stable 120-VBlank windows:

| Intro scene | Before: presentations | After: presentations |
| --- | ---: | ---: |
| Opening crawl / animated Chakan | 60 | 120 |
| Story panel 2 | 60 | 120 |
| Story panel 3 | 60 | 120 |
| Story panel 4 | 60 | 120 |
| Hourglass crawl | 60 | 120 |

`check_audio.py --intro --min-render-fps 60` asserts the presentation rate in all
five windows separately, while also checking full intro playback and audio.
This measures rendered frames rather than simulation ticks. Scene asset loads
occur during black transitions; the logo and menu still use their existing
render paths. These results are from PicoDrive, with hardware verification
still pending. Host tests compare prepared-screen pixels with the original
tile/sprite renderer across text scrolling, every animation frame, all panels,
and the two independent SH-2 row ranges.

## Current default: Portal Hub with working entrances

```sh
MARSDEV=/opt/toolchains/mars make GAME=PORTAL_HUB
```

`Chakan.32x` enters Portal Hub after the logo, intro, and Start Game selection. Press **Up** while overlapping an Earth,
Fire, Water, or Air portal to enter its migrated first terrestrial stage. The
Hub fades out over 20 simulation ticks; the destination loads its own player,
collision, camera, graphics, fade-in, and music without resetting the console.
The original debug spawns in each destination remain unchanged. Press
**Start + A + C** together in any terrestrial stage to fade back to the Hub
and restore its spawn and music. The chord is consumed before gameplay jump/roll
handling and triggers once per press, including when held through loading.
Stage-completion/progression and return entrances are not migrated.

Entrance selection follows the source: collision door IDs 1/2/3/4 map to
Air/Earth/Fire/Water, then visible sprite bounds provide a fallback. The current
source map contains no ID 1 cells, so Air uses its actual sprite entrance. ID 5
is the elemental gate and remains inactive. Interaction requires a fresh Up
press; holding Up while approaching an entrance does not activate it. The input
edge state is retained across loading to prevent held buttons acting as new presses.

All five levels coexist in ROM. Their fully reloaded asset arrays use NOBITS
linker overlays at the same SDRAM address; only the active level occupies RAM.
Persistent state stays outside the overlay, and the linker still reserves both
SH-2 stacks. Loaders run after the synchronous render barrier. Scene generations
invalidate the master and slave caches before reusing asset addresses. Hub and
all four songs fit below the fixed 512 KiB audio-mirror limit; the 68000 restarts
the existing hardware-compatible XGM driver with the requested track.

Validation: 20 SH-2 host tests and 7 Genesis tests pass. The transition regression
uses actual Hub/game loaders and checks all door descriptors, sprite fallbacks,
held-Up rejection, fade timing, destination input, music selection, and
Start+A+C returns from all four stages. Incomplete chords do not trigger returns. PicoDrive
checks entered each destination from a Hub save state through normal Up input,
then ran 1200 VBlanks with continued rendering and nonzero audio. Round-trip
checks return using Start+A+C and re-enter each portal to exercise overlay reuse.
Captures were checked for each destination and the restored Hub. Real-hardware portal switching remains unverified.

```sh
python3 md_src/tools/check_audio.py --core /path/to/picodrive_libretro.so --portal-entry earth --return-hub --frames 1200
# Repeat with fire, water, and air. Test-only placement edits a PicoDrive save state;
# no debug teleport command or fixed player address is included in the ROM.
```

### Water renderer optimization

A repeatable standalone 1200-VBlank PicoDrive run improved from 15.211 to
21.316 rendered frames per 60 VBlanks (about 40%). Its one-second interval range
improved from 11..19 to 20..27. This is still below 60 FPS; emulator refresh rate
is not game rendering rate. The same controller harness is used, although
render cadence affects which input edges the game samples.

Water resource conversion now bakes opaque/high background priority into pixel
bit 7, and foreground priority into compact-map bit 15. Pixel alignment moves
background color and coverage together; output strips the tag. Foreground map
lookups mask out the priority bit. Existing untagged formats remain supported.
All source pixel colors, tile priorities, and scroll phases remain unchanged.
The generic background aligner uses constant SH-2 shifts rather than variable
shift helper calls. A measured 128/96 scanline split balances Water's heavier
lower bands; other scenes retain 112/112. The split is published with each job.

The slave's immutable frame snapshot lives in dedicated SDRAM rather than its
4 KiB stack. Compiler stack-usage inspection and the existing linker stack
reservation check cover the optimized build. All 20 SH-2 and 7 Genesis host tests
pass, including pixel comparisons for both tagged and untagged priority,
sub-tile offsets, transparent pixels, opaque-row shortcuts, and the 128-line
render seam. All four portal round trips retain working audio and rendering.

### Audio transitions

Before reloading the XGM driver, the 68000 owns the Z80 bus and explicitly
silences the sound chips: all four PSG volumes, all six FM key states and
operator levels, the DAC (centered then disabled), timers/channel-3 special mode,
and LFO. A Z80 driver reset alone does not reset those chip registers. Stage
Select has no PSG part, so a stage's PSG noise could otherwise survive the
return. Z80 reset remains deasserted during RAM upload, as required for Genesis
bus access, then the existing driver reset/readiness handshake is used.

The PicoDrive round-trip harness checks actual PSG attenuation registers after
return: all four must equal 15 (silent), while Hub audio continues. Earth, Fire,
Water, and Air round trips pass this check and re-entry playback. Real-hardware
confirmation of the transition audio fix remains pending.

### Central Hub camera

The source's inclusive player-top-left region X=704..896, Y=500..800 locks
camera Y at 580 while horizontal following stays active. Initial Hub entry and
Start+A+C return set that height before the fade-in. Entering the region uses
the source's shift-4 approach (1/16 of the remaining distance per tick, minimum
1px); exiting eases back to ordinary vertical following. Arithmetic negative
rounding matches SGDK. Other levels retain their existing camera behavior.
Tests cover initial placement, inclusive boundary points, smooth exit/re-entry,
settling at the lock height, and restoring the lock after a portal round trip.

### Hub scenery

The Hub foreground is synchronized with the current source branch. Its central
emblem sits 16px below the older migrated artwork, aligning the original potion
and hourglass coordinates without sprite-specific offsets.

The four source potion strips animate at their original positions (685,665),
(904,665), (685,702), and (904,694), using Chakan's palette bank and source
foreground priority. Four hourglasses use the source positions (679,606),
(898,606), (679,739), and (898,739). Source artwork remains under
`sh_src/res/sprites/Items/`; these are scenery, without inventory/timer mechanics.

Four planet images under `sh_src/res/sprites/Scenery/Hub/` scroll upward behind
the foreground. A dedicated palette at indices 64..79 preserves the starfield
and player palettes. Planets and items use immutable ROM spans to avoid growing
the level RAM overlay. The sprite budget is 16, with one slot reserved for Chakan;
scene fades include all 80 Hub colors.

There are two horizontal instances of each planet across the full 1920x1280
Hub. Base X/Y positions are (16,10), (260,330), (496,650), and (756,970),
with a second instance of each type 1048px farther right. Neighboring sprite bounds have uniform 212px clear
horizontal gaps, including beside the wider ringed planet. There is only one
vertically wrapping planet per horizontal position. Each planet type
has independent upward animation at 0.75/1/1.25/1.5px per simulation tick,
all faster than the 0.5px starfield drift. Positions wrap from fully above
the level's top to its bottom edge, using the 1280px world height plus the
planet's height. Camera movement only changes the viewport and never triggers
a wrap. Fractional motion survives skipped renders.

## Terrestrial Fire 1 on 32X

```sh
MARSDEV=/opt/toolchains/mars make GAME=T_FIRE_1
```

Fire 1 is available through `GAME=T_FIRE_1`. Level code and collision RLE
follow the source layout under `sh_src/src/levels/terrestrial/fire/`; original
PNG and JASC palettes stay in `sh_src/res/levels/T_Fire/`. The original
`05 - Fire Dimension 1.vgm` is played by the existing Genesis audio service.

- Original 1600x1856 foreground, two independent source palettes, PNG tile
  priority bits, and current source debug spawn (256,100) are preserved.
  Chakan stays behind high-priority foreground tiles. The source has no
  background map or animated scrolling; the renderer uses a black backdrop.
- All 116 collision rows remain hexadecimal `"0xTTCC"` words, 100 cells per
  row, with all eight source tile definitions. IDs 3..5 are explicitly
  one-way 8h platforms (including left/right partial-width ends); IDs 2 and
  6 are ordinary floor-only 8h profiles. ID 7 preserves the solid bottom seal.
  Existing crouch, roll, jump, and drop-through gameplay is shared.
- Asset reconstruction tests cover every source pixel and priority tile;
  host checks cover camera bounds, backdrop, platform flags/heights, and
  actual player drop-through. PicoDrive's 1200-frame run passed audio and
  world-bound checks, measuring a steady 30 rendered frames per 60 VBlanks.
  Real-hardware validation remains to be done.

## Terrestrial Water 1 on 32X

```sh
MARSDEV=/opt/toolchains/mars make GAME=T_WATER_1
```

Water 1 is available through `GAME=T_WATER_1`. Its code and hexadecimal RLE
follow `sh_src/src/levels/terrestrial/water/`; unchanged source PNGs stay in
`sh_src/res/levels/T_Water/`. `09 - Water Dimension 1.vgm` stays under
`md_src/res/music/`. Other levels remain available through `GAME=...` and
explicit VS Code build tasks.

- Original 6400x496 foreground, 512x496 background and spawn (8,250).
  All 31 collision rows are copied unchanged as `"0xTTCC"` runs, 400 cells
  per row. All 27 source tile definitions and their directional flags are
  preserved, including the eight explicitly one-way profiles (IDs 8..11 and
  15..18), the partial-width tile 13, and floor-only profiles.
- The source's world-space raster bands and 8.8 scroll rates are preserved:
  camera X/2 through world Y=288; a per-scanline gradient from 0.5 to 2 pixels/tick
  over Y=289..383; and 2.5 pixels/tick from Y=384 downward. The earlier 81%
  drift tuning was removed after hardware testing reported a two-position
  flicker in the lower water. Its near-integer 2.02px/tick cadence was a possible
  contributor; the restored lower band alternates two- and three-pixel steps. Drift adds to camera parallax,
  background Y follows camera Y, and X wraps across 512 pixels. The immutable
  frame snapshot includes all 224 scanline offsets.
- Priority decoding follows the source's enabled base priority on both planes.
  SGDK `map.c` adds base tile attributes: adding priority 0x8000 flips the
  resource priority bit on 16-bit overflow. Priority masks therefore invert
  PNG bit 7 here. The renderer preserves high background over low foreground,
  high foreground over background, and low-priority Chakan behind either high
  plane. Color-zero pixels remain transparent to sprite priority.
- Compact background rows and the large foreground/map remain in SDRAM.
  Chakan's immutable span descriptors stay in ROM to make room; its pixels
  and row directory remain in SDRAM. Supplemental crouch/roll frames also
  remain in ROM. The linker still reserves both SH-2 stacks.
- The source has no active Water 1 enemy/exit hooks in its processing loop;
  this slice retains the shared player movement, crouch, roll and drop-through.

Validation reconstructs both PNGs and both priority maps, checks all RLE rows,
exercises 513 ticks of all raster-band boundaries, and checks the 27 collision
flags plus a drop onto a solid floor. The actual C rasterizer is compared
pixel-for-pixel for per-line scrolling, compact background data, both plane
priorities and mixed sprite depths. PicoDrive captures at 120/300/600/1200
show the original scene, and the 1,200-frame run has nonzero audio in both
600-frame intervals. This level still needs rendering optimization and
real-hardware confirmation. Current scripted throughput averages about 15
rendered FPS. All 24 host tests pass.

## Terrestrial Air 1 on 32X

```sh
MARSDEV=/opt/toolchains/mars make GAME=T_AIR_1
```

Select `GAME=T_AIR_1` to boot T_Air_1 in `Chakan.32x`. Its code and collision
JSON follow `sh_src/src/levels/terrestrial/air/`; unchanged source images and
JASC palettes live in `sh_src/res/levels/T_Air/`. The build uses only this
checkout. `GAME=T_EARTH_1`, `GAME=PORTAL_HUB`, `GAME=TEST_WIDE` and
`GAME=TEST_SMALL` remain available, with explicit VS Code build tasks.

- Original 1920x1440 foreground, 256x224 background and spawn (464,64).
  Both foreground palette banks are retained, with source priority-bit tiles
  occluding Chakan. Character/background palettes occupy separate 32X banks.
- All 90 collision RLE rows are copied unchanged as hexadecimal `"0xTTCC"`
  strings. IDs 0/4 are empty; 1/3 are full solids; ID 2 is the source's 9h
  heightmap with all four directional blocking flags. It cannot be dropped
  through. Shared player crouch, rolling and collision support remain active.
- The source's 28 visible tile-row scroll bands run independently of foreground
  scrolling. Their base X follows camera movement at 1/8 speed. Rows 11..16
  have no autonomous drift; the outer bands drift right at 0.25..2.75 px/tick
  using 8.8 phases. Background Y stays zero. The complete row table is copied
  with each immutable master/slave frame snapshot.
- `11 - Air Dimension 1.vgm` is converted to XGM at build time and played by
  the existing 68000/Z80 service with the hardware-confirmed ROM banking fix.
- Air's additional priority mask and the terrain, background and original
  Chakan movement frames fit in SDRAM. Action sprites remain in ROM. Data ends
  at `0x06036680`, below the reserved stack boundary `0x0603E000`.
- No enemies or level exit transitions are added; source Air 1 leaves its
  entity/trigger hooks commented out.

Validation includes pixel reconstruction against both original images, all
priority bits, hexadecimal RLE dimensions, 513 ticks of autonomous band phases,
solid 9h collision, renderer row scrolling and mixed sprite priority. The
PicoDrive 1,200-frame run produces music in both 600-frame windows, progresses
through the level and keeps player/camera in bounds. Captures at frames
120/300/600/1200 were inspected. Rendering still needs optimization; the
current Air scene averages about 28 rendered FPS in the scripted run and does
not sustain 60 FPS. All 22 host tests pass. Real-hardware confirmation is pending.

## Terrestrial Earth 1 on 32X

```sh
MARSDEV=/opt/toolchains/mars make GAME=T_EARTH_1
```

Select `GAME=T_EARTH_1` to output `Chakan.32x`. Gameplay and
all visuals use the SH-2s and 32X VDP. The 68000/Z80 service plays the original
`07 - Earth Dimension 1.vgm`, converted to XGM at build time, using the ROM
mirror banking fix already confirmed on hardware for Portal Hub.

- Source layout is preserved in `sh_src/src/levels/terrestrial/earth_levels.c`
  and `sh_src/res/levels/T_Earth/`. The two PNGs are unchanged copies.
- The actual foreground is 2576x864, and every original RLE row contains
  161 cells. The source level metadata says 2560/160; this port uses the
  actual art/RLE dimensions, preserving the extra rightmost column.
- All 54 collision rows remain hexadecimal `"0xTTCC"` runs in
  `t_earth1_collision.json`. Spawn remains (0,160) in sprite-origin space.
- Tile 1 is solid on every side. Tiles 2/3 are floor-only 16h/8h geometry;
  tiles 4..7 use the source's four 2w slope heightmaps. IDs 8/9 explicitly
  carry `TILE_FLAG_ONE_WAY` and can be dropped through. All 54 RLE rows now
  match `create_level_stages` (18 changed from master).
- The source collision resolver, geometry/behavior descriptors and directional
  flags now live under `src/level_collision/` and `inc/level_collision/`.
  Earth, Hub and TEST_WIDE share it. The source's window allocator is replaced
  by bounds-checked lookups into the resident grid; no SGDK allocator is needed.
  Support-point floors, 4px step-up, 8px step-down, 6px ceiling nudging,
  solid headroom tests and behavior reporting are retained. A swept floor
  crossing check also catches fast falls through thin platforms. Collider
  max edges passed to the resolver are inclusive. World sides/top clamp;
  the port retains respawn below the map.
- The original 256x224 background has independent scroll accumulators driven
  by camera deltas: half speed horizontally, full speed vertically. Its own
  offsets remain separate from foreground offsets. Player movement inside the
  camera dead zone or beyond an edge clamp does not scroll the background;
  fractional horizontal movement is retained when the camera moves. GDL now supports non-power-of-two background heights;
  both rasterizer paths are tested across the repeat boundary.
- Chakan retains movement, animation, jumping and camera tracking. The
  level fades in over 20 ticks and respawns the player after falling below it.
- No enemies or level-completion transitions are added: the source Earth 1
  processing loop has those hooks commented out.

Controls from `create_level_stages`:

- Down while stationary and grounded crouches. Standing height is 58px
  (source sprite offset 22); crouching/rolling height is 40px (offset 40).
  Feet stay fixed when changing height. Low solid ceilings force crouching
  until there is room to stand; explicitly one-way tiles are ignored by this test.
- Down + A/B/C first tries a drop through the supporting one-way floor.
  Mixed solid/one-way support rejects the drop. The 12-tick ignore window skips
  only `TILE_FLAG_ONE_WAY` geometry, preserving solid floors underneath.
- Otherwise, crouch/Down + A/B/C rolls in the facing direction at 8px/tick,
  up to 137px. Releasing the jump buttons cancels the roll; holding them
  cannot repeatedly trigger it. Disabling controls/simulation clears actions.
- The unchanged 896x560 sprite sheet from this branch adds crouch and four
  roll poses. Five supplemental action frames stay in ROM; the original
  twelve frames remain in SDRAM. All three SH-2 levels bind both image sets.

Validation: 20 host tests pass, including every foreground/background source
pixel, collision row widths, actual C one-way/slope movement, and renderer
wrapping. A 1,200-frame PicoDrive run produces music in both 600-frame intervals,
with progressing SH-2 rendering and bounded camera/player positions. Captures
at frames 120/300/600/1200 were inspected. Current measured render throughput
averages about 48 FPS under the current scripted run; this level is not yet at 60 FPS.
SH-2 data ends at `0x0603CA40`, below the reserved stack boundary `0x0603E000`.
A separate 600-frame controller sequence crouches, rolls onto the starting
one-way ledge, then drops through it; captured poses were visually inspected
and audio stays nonzero. Earth 1 still needs confirmation on real hardware.

Mode-switch checks also built and ran Portal Hub, TEST_WIDE and TEST_SMALL
with nonzero audio. The SH-2 checks reached the original Hub/test rightmost
camera bounds. The final ROM is restored to T_EARTH_1.

Other selectable builds remain `GAME=PORTAL_HUB`, `GAME=TEST_WIDE`, and
`GAME=TEST_SMALL`.

## Portal Hub on 32X

```sh
MARSDEV=/opt/toolchains/mars make GAME=PORTAL_HUB
```

Select `GAME=PORTAL_HUB` to build the Hub. Output remains `Chakan.32x`.
Portal Hub code follows the original relative layout under
`sh_src/src/levels/hub/`; source art stays under `sh_src/res/levels/Hub/` and
`sh_src/res/sprites/Scenery/Hub/`.

Implemented content:

- Original 1920x1280 foreground with both terrain palettes, repeating starfield
  and its vertical drift, and original spawn (756,720) in sprite-origin space.
- Original 120x80 collision grid, with all RLE runs preserved unchanged. ID 1 is
  solid; collision-map IDs 2..6 preserve the portal markers. Collision JSON
  stores each RLE word as a four-digit hexadecimal string (`"0xTTCC"`):
  `TT` is the tile index and `CC` is the consecutive cell count in that row.
  For example, `"0x010A"` means ten cells of tile 1. This format is shared by
  Portal Hub, TEST_WIDE and TEST_SMALL; runs never cross row boundaries.
  The source defines
  their non-solid `TILE_BEHAVIOR_DOOR` descriptors in
  `src/level_collision/collision_heightmaps.c`: Air=1, Earth=2, Fire=3,
  Water=4, Elemental=5 (behavior IDs). The SH-2 port now includes these
  descriptors and overlap reporting. `collision_player_behavior()` exposes
  type/ID/param; `player_collision_result` also exposes collision flags and
  hit coordinates. Destination transitions are not yet implemented.
- Original Chakan animations and collider, SH-2 movement, bounded two-axis camera
  (x=0..1600, y=0..1056), and respawn after falling out of the world.
- Four portal sprites at original positions, nine flames with six-frame
  animation, camera culling and source-style foreground occlusion. The player
  remains in front of terrain. Planet routines are not called by the source Hub
  and were not added to its scene.
- Twenty-tick fade-in, followed by palette cycling of colors 27..31 every eight
  updates. Palette changes queue for the next completed VBlank presentation;
  direct palette loads wait for the VDP palette-access flag.
- Stage Select music continues through the existing 68000/Z80 audio service.

The new converter packs duplicate foreground rows and duplicate opaque sprite
runs. Terrain row storage falls from 125440 bytes to 58864 bytes, making room
for the complete 76800-byte map in SDRAM. All Hub drawing resources are resident
before frame submission. The linker reserves the top 8KiB for master/slave
stacks and rejects data overlaps; the current data end is around 239KiB.
The generated header is `sh_src/res/portal_hub_data.h`.

Shared player animation selection now lives in `sh_src/src/player_sprite.c`.
GDL supports an optional compact tile-row directory and a per-sprite
`behind_foreground` flag. Its direct-row fast path remains separate for the
existing test renderer. The scene still uses both SH-2s and the 32X framebuffer;
Genesis graphics are cleared and do not draw the Hub.

### Hub validation and remaining limits

```sh
python3 -m unittest discover -s sh_src/tools
python3 -m unittest discover -s md_src/tools
python3 sh_src/tools/convert_portal_hub.py --check
python3 md_src/tools/check_audio.py --core /home/christian/.config/retroarch/cores/picodrive_libretro.so --portal-hub --frames 1200
```

Tests reconstruct the compact terrain against the original image, verify both
palette banks, all collision row widths, preserved door markers, and every
scenery sprite in both facings. The actual C renderer is checked for compact
rows and foreground occlusion. A scripted PicoDrive traversal reaches player
x=1878 and camera x=1600 while audio continues; captured frames were inspected.

Portal Hub does **not** yet sustain 60fps through all tested views. The current
1200-VBlank traversal averages approximately 54.3 rendered fps after warmup,
with sampled intervals ranging 39–60. Simulation follows the VBlank clock with
bounded catch-up. Further optimization is needed for dense terrain and scenery;
RetroArch's output counter alone cannot detect those missed render intervals.
TEST_WIDE's comparison run remains above 59.9 rendered fps. Real hardware/PAL,
portal destination levels, menus and sound effects remain unverified or unported.

## TEST_WIDE comparison (previous default)

`GAME=TEST_WIDE` builds the comparison scene with both SH-2 processors and the 32X VDP.
The Genesis `TEST_SMALL` version remains selectable without cleaning:

```sh
MARSDEV=/opt/toolchains/mars make GAME=TEST_WIDE
MARSDEV=/opt/toolchains/mars make GAME=TEST_SMALL
```

Both use the existing `Chakan.32x` output and VS Code run task. The default
release/debug tasks now select PORTAL_HUB; a separate VS Code task builds Genesis
TEST_SMALL. A generated `build/game_config.h` tracks backend changes so switching
recompiles both CPU entry points and rebuilds the embedded 68000 image.

The original project's layout is mirrored under `sh_src/src/`, `sh_src/inc/`,
`sh_src/res/` and `sh_src/tools/`. The wide-level converter and its PNG decoder are local to `sh_src/tools/`;
backend configuration also lives there. The earlier Genesis implementation stays
under `md_src/`. The sections below describing TEST_SMALL remain its baseline
reference; the default ROM's current architecture is described here.

### Processor ownership

| Processor | Work in TEST_WIDE |
| --- | --- |
| Master SH-2 | Portable player/input logic, original solid collision map, deadzone camera, animation, starfield position; draws rows 0–111 |
| Slave SH-2 | Draws rows 112–223 from the master's immutable render snapshot |
| 68000 | Polls controllers, publishes input and VBlank ticks, runs the XGM host service; clears Genesis graphics at startup |
| Z80 | Plays the original Stage Select music through the Genesis sound chips |

The game image uses the 32X VDP's 8-bit indexed, double-buffered 320×224 mode.
Both processors write disjoint word-aligned scanlines of the hidden framebuffer.
The master publishes jobs through uncached SDRAM, waits for slave completion,
then flips buffers. Maps, tiles and sprite frames are copied into SDRAM at init.
Genesis hardware sprites and planes do not render the wide scene.

The 68000 remains necessary for Genesis I/O and the existing Z80 audio service;
player simulation, collision and rendering no longer execute there in TEST_WIDE.
The boot template incorrectly used r0 after its initializer call when releasing
the slave. Restoring the communication-register base fixes actual slave startup.

### Wide-level content and limits

- Original `fg_test_level_wide.png` (1440×240), starfield and Chakan sheet are
  copied unchanged at matching relative resource paths.
- The 90×15 solid collision grid and spawn (20,40) come from original
  `src/levels/test/test_wide.c`. Its final two empty rows encode `0x0090`
  (144 cells), exceeding the declared 90-cell width. The local JSON normalizes
  those to `0x005A` (90 empty cells); all other runs are preserved.
- Player/input code under `sh_src/src/` is ported from the tested 68000 version.
  Positions remain collider-space, with the original 42×56 collider and sprite
  offset. Both versions pass the same portable movement harness.
- The original follow-camera deadzone (156,104)–(164,120) is applied to the
  collider center. Camera bounds are x=0..1120, y=0..16 for the 224-line viewport.
- The starfield wraps horizontally and drifts vertically by half a pixel per
  simulation update, including the original camera-Y contribution.
- Stand/walk/jump/fall/landing frames and horizontal mirroring are rendered as
  indexed pixels rather than Genesis hardware sprites.
- Simulation catches up to the 68000 VBlank clock, bounded to four steps after
  a delay. The optimized C software renderer now sustains one rendered frame per VBlank
  in the tested NTSC PicoDrive sequence (60 fps); the original version achieved
  roughly 25 fps. The four-step catch-up remains as protection against stalls.
- Animated portal entities/transitions, palette cycling, full slope/one-way
  collision, menus and sound effects remain outside this first SH-2 slice.
  The foreground includes its original static portal scenery.

### Verification

```sh
python3 -m unittest discover -s md_src/tools
python3 -m unittest discover -s sh_src/tools
python3 sh_src/tools/convert_test_wide.py --check
python3 md_src/tools/check_audio.py --core /home/christian/.config/retroarch/cores/picodrive_libretro.so --wide --frames 1200
```

All six existing checks and five wide checks pass. The latter reconstruct
source tiles, validate collision row widths and run the SH-2 portable movement
and camera code on the host with undefined-behavior checks. In the optimized 7200-frame PicoDrive run, 7177 dual-processor frames completed
(including the startup delay). After the first 60-frame warmup, every interval
completed exactly 60 renders per 60 VBlanks. The camera reached x=1120, the
collider reached its right boundary x=1398, and audio remained nonzero.
Captured frames were inspected at different camera positions, including the
far end of the level. TEST_SMALL was rebuilt and its audio smoke check passed;
then TEST_WIDE was restored as the current ROM. Real hardware and PAL remain
unverified.

## GDL32X extraction

The working renderer is now a reusable module under `sh_src/inc/gdl/` and
`sh_src/src/gdl/`. See `sh_src/src/gdl/README.md` for the API, resource format,
ownership rules and current limits. Drawing, video and dual-processor scheduling
are separate from the game. `TEST_WIDE` owns assets, collision, camera and
animation and submits a scene/frame description. The pure rasterizer has no
MarsDev register or game-header dependency and can execute in host tests.

The first API supports the current two-layer indexed renderer and up to eight
span sprites with arbitrary image dimensions and frame counts. It preserves
region-based parallelism; producer/consumer queues, persistent framebuffer
caching and a 384x320 virtual canvas are not part of this extraction.

A new host harness verifies actual C rasterizer output against a pixel reference,
including both facings, overlapping sprites, clipping, all camera byte alignments,
row splits and padded surfaces. All twelve checks pass (six MD/tools and six
SH/tools). The extracted version sustained exactly 60 renders per measured
60-VBlank interval after warmup in a 7200-frame PicoDrive run, with uninterrupted
nonzero audio. At that migration stage, the default output was the SH-2 `TEST_WIDE` ROM.

## Renderer optimization and timing

The profiled baseline spent about 34ms drawing on each SH-2; gameplay was below
1ms. The optimized renderer preserves resolution, pixels, animation and camera:

- Foreground tile rows carry precomputed transparency masks and empty/opaque/
  mixed classifications. Empty rows copy the background directly, opaque rows
  copy the foreground, and mixed rows compose with two 32-bit mask operations.
- Background tiles already contain their final palette indices.
- Map entries carry precomputed row-base offsets instead of runtime multiplies.
- Chakan uses opaque spans for both facings, skipping transparent pixels and
  avoiding per-pixel palette calculations and mirroring.
- Row composition and framebuffer output use aligned 32-bit words. Four fixed
  shift paths handle every camera byte alignment. Output loops are unrolled.
- SDRAM work data dropped from roughly 134KB to 80KB, and the ROM from 168KB
  to 112KB. No resolution reduction or frame skipping is used.

The longer run averaged approximately 11.6ms master drawing and 11.3ms slave
drawing, with measured maxima around 14.6ms and 14.8ms. Timing is approximate:
the 68000 samples the V counter in its service loop and publishes a monotonic
scanline clock. This avoids reliance on FRT, which did not advance in this
PicoDrive configuration. NTSC scanlines are approximately 63.6 microseconds.

`sh_src/inc/profile.h` reads that clock. `sh_src/m_main.c` records average and
maximum durations for simulation, master drawing, slave drawing, master wait
and buffer-flip wait. Uncached job data carries slave timing. The 68000 copies
packed profiling messages to `sh2_profile[10]` for external inspection. Values
0–4 are averages and 5–9 are maxima, in scanlines; durations saturate at 1023
scanlines in the diagnostic transport. Profiling does not appear on screen.
The startup handshake now explicitly waits for the 68000 service loop, so the
shared clock register cannot race a one-time readiness marker.

Reproduce the throughput check with:

```sh
python3 md_src/tools/check_audio.py --core /home/christian/.config/retroarch/cores/picodrive_libretro.so --wide --frames 7200 --min-render-fps 59.9
```

The check measures completed SH-2 frames at 60-VBlank intervals, discards the
first interval as warmup, and reports mean/min/max throughput. Video callback
count alone is not used as evidence of rendering performance. Audio remained
nonzero in all twelve 600-frame periods. Real hardware, PAL and other cores
remain unverified; this is an emulator measurement, not a hardware guarantee.

## Project layout

Preserve the original SGDK project's relative layout inside `md_src` as files
are migrated: `src/`, `inc/`, `res/` and `tools/`. Retain source filenames and
subdirectories such as `src/levels/test/test_small.c` and `res/levels/Hub/`.
Only ported code is compiled; unported SGDK modules will be brought over into
matching paths as their dependencies are replaced.

The MarsDev boot assembly and linker script remain at `md_src/md_start.s`,
`md_src/font.s` and `md_src/md.ld`. Platform declarations live in
`md_src/inc/common.h`. The 68000 entry point is `md_src/src/main.c`.
The temporary scene implementation and header live together in
`md_src/src/levels/test/`, matching the original test-level layout. Generated
resource data lives in `md_src/res/test_small_data.h` and is not checked in.

The ROM target is `Chakan.32x`; VS Code's build-and-run task uses that filename.
The ROM's domestic, overseas and 32X module titles identify Chakan Remake.

## Current slice

- `md_src/src/levels/test/test_small.c` draws the original `TEST_SMALL` foreground and starfield on
  Genesis planes A and B. Chakan now uses the original animated 112×80 sprite frames.
- Local source assets and original RLE collision rows live in
  `md_src/res/levels/Hub/` (images) and
  `md_src/src/levels/test/test_small_collision.json` (collision); the sibling SGDK checkout is not required to build.
- `md_src/tools/convert_test_level.py` generates uncompressed palettes, deduplicated
  4bpp tile patterns, tilemaps and a collision grid using Python 3 alone.
- Foreground: 86 tiles; starfield: 79 tiles. Patterns use indices 16–180,
  well below plane A at `0x8000`. Palettes 0/1/2 serve foreground/background/player.
- Left/right move; A, B, or C jump. A temporary per-pixel solid-cell solver
  handles the original map's floors, walls and ceilings. Falling below the
  240-pixel world respawns the placeholder at the original spawn (10, 40).
- The starfield scrolls vertically by 0.5 pixels per update, matching the original
  small scene. Only plane B VSRAM changes; foreground remains fixed.
- V28 remains enabled: 224 visible lines, with the bottom 16 world pixels
  outside the viewport. Camera support is not yet integrated.
- Original fixed-point movement and gameplay input are now adapted in
  `md_src/src/player.c` and `md_src/src/input_controls.c`, with headers under
  `md_src/inc/`. Portals, palette cycling and sound effects are still pending. Input indicators were removed to avoid
  overwriting the new background.
- C header dependency files are generated for both CPUs.

Build with the MarsDev toolchain location for this machine:

```sh
MARSDEV=/opt/toolchains/mars make
```

## Validation and next steps

```sh
python3 -m unittest discover -s md_src/tools
python3 md_src/tools/convert_test_level.py --check
MARSDEV=/opt/toolchains/mars make -B
```

Four level converter tests, sprite reconstruction tests, the host player/input
behavior harness and the ROM build
pass. The harness uses the host C compiler with undefined-behavior checks.
`Chakan.32x` is now 57344 bytes. The existing RAM-code `.data` warning remains.
The user confirmed foreground/background rendering, placeholder rendering and
apparently working collision in the emulator before the layout reorganization.
The reorganization was verified by rebuilding and running the converter tests.

Next, port original collision geometry, the camera and wide test level, and
integrate a sound driver. The assessment
below records the prototype baseline before this implementation.

## Player/input migration

The movement port retains the source's six-bit fixed-point velocity, speed 2,
acceleration 0.25, deceleration 0.2 (truncated to 12/64), gravity 0.5, jump
impulse 12 and source terminal-velocity gating. Positions are updated using
integer velocities as in the source, including floor rounding for negatives.
The source's left/right acceleration asymmetry is retained.

Jump input is based on button transitions. Holding A/B/C does not automatically
jump again after landing. A jump press is buffered for five frames; grounded
contact refreshes five frames of coyote time. Releasing all jump buttons halves
upward velocity once, allowing shorter jumps. Right takes priority when both
horizontal directions are held. Control and simulation enable/disable helpers
are retained; simulation disable freezes movement and clears pending jumps.

`did_jump` is now set when an impulse is applied; the source declared this flag
but never set it. Rendering remains in the test-level module. It supplies a
collision callback to the player, so host tests run the same movement code
without accessing hardware registers.

This is the movement/input slice, not the full SGDK collision or sprite port:

- The original 42×56 collider is now active. Player coordinates represent its
  top-left; the sprite origin is offset by (-35,-24). Level spawn is adjusted
  by (+35,+24) to retain the original sprite-space spawn position.
- Keep the solid-grid collision solver with per-pixel axis stepping and clear
  velocity on wall/ceiling contact. Slopes, one-way geometry and collision
  windows are pending.
- Keep fall-and-respawn behavior in the level rather than introducing SGDK's
  hard-reset/death flow. Hold state survives respawn to avoid synthetic presses.
- Menu/pause handlers and animations are not yet connected.

The harness checks acceleration, ground friction, airborne momentum, held jump,
release cutoff, each jump button, buffered landing, coyote expiry, wall/ceiling
blocking, gravity cap and control/simulation gating. Emulator movement validation
for this slice is still pending: compare held and tapped jumps, ledge jumps and
jump presses immediately before landing.

## Chakan sprite and starfield

`md_src/res/sprites/Chakan/chakan_anim_no_weapons.png` is copied unchanged from
its original path. `md_src/tools/convert_sprites.py` generates `res/sprites.h`
with 414 unique tiles and 12 frames: stand, eight walk frames, jump, fall and
landing. Tile indices 192–605 are reserved for Chakan; the level converter
rejects overlap. All tiles load once with the display disabled.

`md_src/src/player_sprite.c` builds a linked SAT from nonempty 8×8 pieces.
Every frame stays below the 80-sprite screen limit and 20-sprite H40 scanline
limit; the converter checks both. Horizontal facing mirrors piece positions
and tile attributes. Walking advances every eight updates. The short landing
pose is selected on the fall-to-ground transition. Tests reconstruct every
frame from generated tiles and SAT pieces and compare it to the source image.

The background's half-pixel accumulator wraps every 512 updates, corresponding
to its 256-pixel height. Chakan art, original collider and vertical scrolling
need emulator validation after this slice.

## Audio integration

Stage Select music now starts automatically after level initialization.

- `md_src/res/music/02 - Stage Select.vgm` preserves the original source path.
- MarsDev's `xgmtool` compiles it with NTSC timing into a generated XGC binary.
- The SGDK XGM driver and its three assembly includes are vendored under
  `md_src/src/snd/` and `md_src/inc/snd/`, retaining the original authorship
  comments. They were copied from this machine's MarsDev SGDK installation.
  `sjasm` rebuilds the driver; no prebuilt driver binary is required in Git.
- `md_src/res/music.s` embeds the song on a 256-byte boundary, a silent PCM
  sample and the assembled driver.
- `md_src/src/snd/sound.c` adapts the driver upload, sample table and music
  commands from SGDK's `xgm.c` / `z80_ctrl.c` without linking its full runtime.
- The host passes `0x880000` ROM-mirror addresses to the Z80. Boot leaves RV
  clear so the SH-2s can access ROM; the low cartridge window is unavailable
  in that mode. Earlier builds incorrectly passed low cartridge offsets,
  which played in PicoDrive but were reported silent on real hardware.
  The XGM `setBank_BC` and `setBank_DE` macros now select the fixed
  `0x880000..0x8FFFFF` mirror, including address bit 23, with their original
  101/105-cycle timing preserved. A linker assertion keeps audio in that
  512 KiB window. The bank-serialization test checks every 32 KiB bank in
  the window and both cycle budgets. The rebuilt Portal Hub ROM passed
  1,200 PicoDrive frames with nonzero audio in both 600-frame intervals;
  confirmation on real hardware is still pending.
- Driver upload occurs with reset released and the bus held. Reset is asserted
  after upload, then released to start the driver. Readiness polling is bounded;
  a failed driver leaves gameplay running with `sound_ready == false`.
- `sound_update()` queues music ticks once per game frame, respecting the
  driver's pending-frame modification handshake. PAL frames queue 60 music
  ticks per second using a tempo accumulator; PAL playback is not yet tested.
- Controller reads now wait for Z80 bus acknowledgement before reading input,
  then release the bus so music can continue.

Build still uses `MARSDEV=/opt/toolchains/mars make`. The MarsDev installation
must provide `xgmtool` and `sjasm` in its m68k tool directory. `make clean`
removes generated music and driver binaries so both regenerate on the next build.

The original player has its jump PCM setup/playback commented out; no jump
sample has been added. The original scream and lightning effects remain pending
until their associated states are ported.

### Audio verification

```sh
python3 md_src/tools/check_audio.py --core /home/christian/.config/retroarch/cores/picodrive_libretro.so
```

This headless smoke check ran 7200 frames (about two minutes) with scripted
controller input and captured 10584000 audio samples. Every 600-frame interval
contained nonzero output; peak absolute sample amplitude was 6582. The ROM also
continues generating video frames. This verifies sustained emulator audio,
including controller bus activity, but does not establish subjective sound
quality or real-hardware/PAL compatibility. Listening in RetroArch is the next
validation step.

## Workspace assessment (2026-10-21)

Source inspected: `/home/christian/projects/Genesis/Chakan_Remake_v1/`.
The source currently defaults to `PORTAL_HUB` in
`src/gamestates/gamestate_play.c`; the small test level is a deliberate first
porting target, rather than the source game's default scene.

A forced rebuild with `MARSDEV=/opt/toolchains/mars make -B` succeeds and
produces `hello.32x` (8192 bytes). The linker-time assembler warning about
changed `.data` section attributes remains. Frequently called functions are
placed in `.data` to execute from 68000 RAM, following the template convention.
This assessment did not repeat the emulator or visual/input validation.

### First playable migration target

Use `src/levels/test/test_small.c` as the reference:

- World: 320 by 240 pixels; spawn at (10, 40).
- Collision: 20 by 15 cells, each 16 by 16 pixels, encoded as RLE rows.
  Collision IDs and geometry are separate from graphical tile indices.
- Foreground: `res/levels/Hub/fg_test_level.png`, defined in
  `res/backgrounds.res` as `ts_test_level_floor` / `map_test_level_floor`.
- Background: `res/levels/Hub/portal_starfield.png`.
- Player: `res/sprites/Chakan/chakan_anim_no_weapons.png`, defined in
  `res/sprites.res` with 14 by 10 tiles per animation frame (112 by 80 pixels).
  This requires multiple Genesis hardware sprites per frame.
- The full source scene also initializes portals, plays stage-select music,
  scrolls its starfield and cycles its palette. These are subsequent features.

### Dependencies to replace

| Source subsystem | MarsDev work needed |
| --- | --- |
| `.res` resources | Linux asset generation for palettes, 4bpp tiles, tile attributes and animation data; no generated SGDK structure dependency |
| `VDP_*`, `PAL_*`, `MAP_*` | Tile/palette upload, plane drawing, scrolling and explicit VRAM allocation |
| `SPR_*` | Hardware sprite composition, animation timing, flipping and tile uploads |
| `player.c`, `input_controls.c`, `game_entities.c`, `types.c` | Portable types/fixed-point helpers, input transitions, original movement and collision integration |
| `level_collision/collision.c` and heightmaps | Preserve RLE and geometry semantics; replace `MEM_alloc` / `MEM_free` or use bounded static buffers |
| `camera.c`, level loading and game states | Adapt frame ownership and plane scrolling after the small scene works |
| `XGM_*` | Separate sound-driver and music/SFX conversion work |

The original movement includes fixed-point motion, five-frame coyote time and
jump buffering. The prototype's integer gravity, small collider and held-button
jumping are temporary behavior, not equivalents of that system.

### Recommended implementation sequence

1. Copy the small level's required source assets into this repository and add a
   reproducible converter. Validate palette indices, tile dimensions, tile count
   and map bounds; integrate generation into `make`. Keep the port build
   independent of the sibling SGDK checkout.
2. Render its foreground and starfield with the placeholder player. Allocate
   tile patterns below plane A at `0x8000` and retain distinct plane, scroll
   and sprite-table ranges. Resolve the source's 240-pixel world versus the
   prototype's display height explicitly when integrating the camera.
3. Bring over its collision rows and geometry, then original movement/input
   behavior. Confirm solid floors, walls, ceilings, jumping and leaving ledges
   before adding animation.
4. Convert Chakan's animation frames and implement hardware sprite composition.
   Verify standing, walking, jumping, falling, landing and horizontal flipping.
5. Add the wide test level to validate camera movement and map streaming, then
   portal/game-state transitions, menus and audio.

Keep the 68000 responsible for the first playable slice. The SH-2 renderer can
be introduced as a separate architectural change once there is a comparable
version of the original scene running.

### Build follow-up

Compiler-generated C header dependencies have now been added. A dedicated
linker section for RAM code can replace the current code-in-`.data` convention
and remove its warning.

The menu recording is now preserved in full by `convert_menu_sound.py`.
Letter reveals start at tick 286 (about 4.76 seconds) and advance at thirteen-tick
intervals to follow its embedded impacts. There are no additional per-letter PCM
commands. The previous extraction/muting approach removed lightning along with
the impacts and has been removed. The full menu recording plays on PCM channel
2, matching the source's SOUND_PCM_CH3, with its own sample slot. Audible
alignment and lightning tails should be confirmed on hardware.

## Hub progression

`level_progress.c` stores 24 stage-completion bits in persistent RAM outside
level overlays. `level_manager_init()` starts a new game and clears them; normal
level changes/Hub reloads retain them. `level_manager_complete_current()` is the
stage-exit API: it marks the current playable stage once and fades to Hub.
The existing Start+A+C return chord abandons a stage without completing it.
The source has no finished stage-exit triggers, so no completion shortcut or
invented exit region was added. Progress is not saved across a reset.

Hub entrances select the first incomplete terrestrial stage for each element.
After all twelve terrestrial stages are complete, they select elemental stages.
Completed groups hide their portal; all elemental stages unlock final-door
routing. All 16 stage and 8 level-clear source markers are converted with PAL1
and their original positions, combined into one sprite to preserve the existing
sprite budget. They update immediately when flags change, including in Hub.

Later stage/final IDs are represented for progression, but the runtime only
loads the four implemented stage-1 levels. Requests for a placeholder stay in
Hub without freezing control, loading empty maps or changing music. The next
stage is not silently replaced with stage 1. The level registry/availability
check can be extended as new content is implemented.

Host tests cover all 24 stage bits, out-of-order/idempotent completion, realm
unlocking, unavailable destinations, final doors, completion versus return,
normal fade/reload persistence, marker count/bounds and new-game reset.

PicoDrive smoke validation navigated Logo → Intro skip → Menu → Play. A test-only
save-state update of completion bits confirmed the complete Hub marker display;
no debug completion command was added to the ROM. All 24 markers are represented
by 200 spans in the composite, leaving the player/scenery sprite budget intact.

```sh
python3 -m unittest discover -s sh_src/tools -p test_level_progress.py
python3 -m unittest discover -s sh_src/tools -p test_portal_transitions.py
```

Water source-rate regression checks verify all scanline offsets and the lower
band's 2/3-pixel cadence across phase wrap. This restores the SGDK timing without
changing priority/opacity. Hardware confirmation of the visual improvement is
still needed; dropped rendered frames can independently cause temporal aliasing.

The title-menu final double lightning flash starts at tick 320 instead of 380,
advanced one second at 60Hz based on hardware audio/visual comparison. Its
two-pulse pattern is retained; the original sound recording is unchanged.

The final title-menu flash now advances through two bright frames, a dark
separation, two bright frames and a final dark frame using completed framebuffer
presentations. Simulation catch-up cannot consume either pulse or its separating
dark frame. It still starts at tick 320; skipping the splash cancels the effect.

## Camera API coverage

`camera.h` now exposes configurable deadzones, lookahead magnitudes/smoothing,
follow/cinematic modes, cinematic center targeting, direct position setters,
vertical approach, deterministic finite-duration shake, position queries and
region queries. The existing six-argument Hub region-lock adapter is retained.
`camera_step_velocity(center_x, center_y, vx, vy)` is wired into every SH-2 level;
`camera_step(x,y)` remains a zero-velocity compatibility wrapper. Camera init
resets all mode, lookahead, shake and background-scroll state.

Defaults preserve this port's 156..164 / 104..120 deadzone and disable lookahead
and shake. Published `cameraPosition` includes shake; logical follow position is
kept separate so shaking cannot cause permanent drift. Use setters to reposition
the camera. Small levels clamp to (0,0), and render-time shake is clamped to valid
viewport bounds on both edges, unlike SGDK's looser VDP output clamp. Smoothing
uses explicit signed floor division, with shifts limited to 15. Lookahead values
are nonnegative magnitudes; invalid deadzones are ignored.

The SGDK Map/VDP helpers are adapted to the software renderer:

```c
camera_apply_foreground(&frame);
camera_apply_parallax(&frame, &scene.background, 1, 2);
camera_apply_bg_b_const_scroll_y(&frame, &scene.background, 32); // 0.5px/tick
```

Background helpers wrap signed offsets to the provided map dimensions, including
negative rates and non-power-of-two heights. Constant Y scroll uses SGDK's six
fractional bits and overrides the frame's background Y when called after parallax.
A zero parallax denominator leaves the frame unchanged. `camera_check_bounds_in_region`
and `camera_get_region_locked_y` retain the source's inclusive top-left test.

Host camera tests cover follow/deadzone, velocity direction and smoothing, mode
changes, cinematics, approach convergence, finite shake/no drift, edge safety,
parallax, reverse/fractional scroll, invalid inputs, tiny maps and region queries.
The existing Hub transition/region-lock and four stage suites remain regression
checks. GCC now emits structure-copy `memcpy` calls in the level loaders; a
freestanding aligned-word/byte fallback is provided alongside the existing memset.

Hub PAL0 now matches `create_level_stages` 07e5f08 byte-for-byte, including entry
2 (`0x08C`, previously `0x0AE`). The foreground PNG's embedded palette and source
master differ at that entry; SGDK loads the explicit PAL0 resource, so it remains
authoritative. PAL1 and starfield already match the gameplay branch. Regression
checks pin both 16-color foreground banks. 32X output still uses the established
3-bit-to-5-bit conversion; transparency and palette cycling are unchanged.

## Pause menu within Play

A fresh Start press during gameplay opens the original 320x224 pause window;
after releasing Start, pressing it again resumes the same loaded session. The
world update is suspended, including physics, camera, level transitions, scenery
and palette animation. Current game state and progression remain intact. Other
buttons do not navigate, confirm or close this initial menu. Start+A+C retains
its return-to-Hub behavior during normal gameplay rather than opening pause.

`pause_menu.c` supplies a prepared screen from the original 4bpp BMP, using
reserved palette indices 80–95. It leaves gameplay palettes and level SDRAM
assets untouched. A dedicated non-inlined pause renderer allocates its temporary
frame only while paused; normal gameplay does not reserve that frame on the
master stack. The window adds no cursor/potion/spell sprites. On resume, input history, jump-held
state and the return chord are synchronized to avoid replaying menu input.

Pause saves the level's complete 8 KiB Z80 RAM and both CPU register banks at
an explicit frame-boundary handshake, then starts Item Selection. The 68000
follows the compiled XGM stream to shadow write-only FM/PSG registers. Resume
restores the saved driver, sample table, PCM buffers, stream position, chip
registers and key gates after the Z80/YM2612 reset, then continues the level
track. The host keeps independent register shadows for menu and level music.
The saved CPU stack uses $1EEC–$1EFF, outside the 64-entry PCM sample table.
Hardware oscillator/envelope phase cannot be restored; held FM notes are
re-keyed on resume. Pause is a mode of `STATE_PLAY`, so state/level
initialization is not run on resume. Standalone stage test backends continue to
bypass the main game-state controller.

Recipes, collection rules and effects are deferred. The intended future
inventory capacity is four each of Air, Earth, Fire and Water (sixteen total).
Potion icons and alchemy spell icons will only appear when their inventory and
combination conditions are satisfied; none are added in this slice.

Validation: host tests verify edge-triggered Start, frozen level updates, other
buttons staying inert, sprite-free pause rendering, input synchronization and
resume without immediate reopening. The Hub transition suite still passes.
PicoDrive captures confirmed the original pause window and restored Hub, and
serialized player state remained unchanged across fifty paused VBlanks.

### Water entry / pause-frame stack regression

The initial pause integration reserved a GdlFrame in `m_main` on every path.
Together with Water's nested priority renderer, this exhausted the master's
4 KiB reserved stack and overwrote game state. Water could stay dark with no
movement even when pause had never been opened. The pause frame now lives in
`render_paused()`, marked noinline to keep it off the gameplay stack. In the
release build inspected for this fix, m_main's local stack allocation decreased
from 1492 to 288 bytes.

PicoDrive checks now cover direct Water entry, Hub pause/resume followed by Water
entry, pausing/resuming inside Water, actual player movement and continuing
frame presentations. All four portal destinations also pass. The permanent
portal smoke harness reserves 600 startup VBlanks, explicitly navigates Intro
and Menu, and rejects destinations that stop presenting frames after entry.

```sh
python3 md_src/tools/check_audio.py --core /path/to/picodrive_libretro.so --portal-entry water --frames 1200
```

Music pause regression: `python3 md_src/tools/check_audio.py --core /path/to/picodrive_libretro.so --portal-entry water --pause --frames 900` checks two pause/resume cycles, independent menu playback, frozen saved level state, resumed song ticks and FM instrument registers against the emulated YM2612.
