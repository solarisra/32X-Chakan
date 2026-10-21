# SGDK to MarsDev port

The first migration slice keeps gameplay on the Genesis 68000 and Genesis VDP,
while retaining the MarsDev 32X ROM boot structure. The SH-2 master initializes
the 32X in transparent 8-bit mode; Genesis-side rendering remains responsible
for the game image.

## Current default: Portal Hub on 32X

```sh
MARSDEV=/opt/toolchains/mars make GAME=PORTAL_HUB
```

`make` now selects PORTAL_HUB by default. Output remains `Chakan.32x` and the
existing VS Code Build/Run tasks use it. `GAME=TEST_WIDE` and `GAME=TEST_SMALL`
remain selectable. Portal Hub code follows the original relative layout under
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
  Water=4, Elemental=5 (behavior IDs). The SH-2 port currently preserves the
  map markers but still needs those behavior descriptors and overlap reporting
  migrated. Destination transitions are a separate step; no wired player
  door-transition handler was found in the source.
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
nonzero audio. Current default output remains the SH-2 `TEST_WIDE` ROM.

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
- The host translates its `0x880000` ROM-mirror addresses into cartridge offsets
  for the Z80. This was verified with PicoDrive's 32X core.
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
