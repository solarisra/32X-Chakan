# GDL32X renderer extraction

This is the first reusable module extracted from Chakan's working SH-2 renderer.
It retains the full-frame, region-based renderer as the performance baseline.
Public headers live in `sh_src/inc/gdl/`; implementation lives here.

| Module | Responsibility |
| --- | --- |
| `renderer.c` | Hardware-independent packed tile composition, clipped software sprites, row output |
| `video.c` | 32X indexed video initialization, palette writes, backbuffer addressing, verified buffer flip |
| `scheduler.c` | Shared asset upload, immutable frame publication, dual-SH-2 ownership and completion barrier |

No player, collision, Chakan animation, respawn or level selection code belongs
in these modules. `src/levels/test/test_wide.c` owns its assets and game state,
selects animation frames, and builds a `GdlFrame`. `m_main.c` owns application
input/timing and profiling transport. The 68000 input/audio service is unchanged.

## Current API

```c
#include "gdl/gdl.h"

/* Master startup, after the platform boot handshake: */
gdl_video_init();
gdl_palette_load(0, palette, palette_count);
/* Upload into caller-owned SDRAM arrays before binding the scene. */
gdl_shared_upload(tile_storage, tile_source, tile_bytes);
if (!gdl_scheduler_init(&scene)) { /* invalid scene */ }

/* Master frame loop, after gameplay prepares the scene description: */
GdlRenderTiming timing;
uint32_t completed = gdl_render_frame(&frame, &timing);
/* completed == 0 means validation failed and no buffer was presented. */

/* Slave entry: */
gdl_slave_loop();
```

`gdl_render_frame` is synchronous to the caller. It publishes a bounded snapshot,
draws the upper half on the master and lower half on the slave, waits for both,
then presents and waits for the hardware flip. Only the master initializes,
submits or presents. Slave startup may enter its wait loop before initialization.
All screen rows are exclusively owned by one worker until completion.

The master draws from the caller's immutable frame for the duration of the call.
The slave copies the active part of the shared snapshot into local memory before
rendering. Ownership counters and shared descriptors use uncached SDRAM aliases.
Scene changes are allowed only between completed submissions. Rebinding does
not reset the sequence and cannot lose the slave's next request.

Assets and scene descriptions remain application-owned. Referenced SDRAM arrays
and immutable image descriptors must remain alive and unchanged while bound.
Uploads happen before publication, never during a frame. There is no allocation
or framebuffer-to-framebuffer copying. Video initialization, asset uploads and
palette loading belong outside the rendering hot loop. Palette writes currently
follow the template initialization behavior; raster-timed palette changes are
not implemented.

## Version 1 capabilities and resource contract

- Fixed 320x224 indexed 8-bit viewport. Pure rasterizer destinations may have a
  padded pitch; the 32X backend uses 80 uint32_t words per row.
- One finite foreground and one repeating opaque background; background X/Y offsets are independent. Background width in tiles must be a power of
  two; height may be any positive tile count (Earth 1 uses 28). The application
  clamps foreground camera bounds before submission.
- Up to eight span sprites, submitted in back-to-front order. Each image defines
  its width, height, frame count and both facings. Clipping occurs before copying
  opaque pixels. All indices are final palette indices; the renderer imposes no
  Genesis palette-bank allocation.
- Tiles are 8x8. Foreground rows have two packed pixel words, two transparent-byte
  masks and a row classification (empty/opaque/mixed). Background rows have two
  packed words. Maps contain uint16_t word offsets, not tile IDs.
- Span rows index triples of uint16_t values (x, length, pixel offset). Image data
  contains all normal frames followed by all mirrored frames. It is independent
  of animation names and timings. Zero is the transparent source convention;
  encoded spans contain only opaque pixels.
- Trusted asset conversion must guarantee map offsets, row offsets, span lengths
  and pixel ranges. Submission validation checks camera bounds, descriptors,
  frame indices and sprite count without scanning every asset each frame.
- Timing values use the existing service clock, approximately NTSC scanlines.

This is an experimental API and asset format. Arbitrary parallax, dirty regions,
virtual framebuffer caching/rebasing, blend effects, raster effects and alternative
CPU schedules remain separate future work. The 384x320 canvas proposal is not
implemented or required by this API. Keep this renderer as the comparison when
benchmarking those experiments.

## Verification

`python3 -m unittest discover -s sh_src/tools` includes a host harness that calls
this actual rasterizer. It compares every output pixel against a simple reference
for camera offsets 0..7, empty/opaque/mixed foreground, a repeating background,
both sprite facings, overlapping sprites, offscreen extremes, row partitioning,
and padded pitches. Guard words detect writes outside the surface and row padding.
The host build uses undefined-behavior checks. Existing conversion checks compare
packed resources and sprite spans to the original PNGs.

The extracted version completed 7176 renders in a 7200-VBlank PicoDrive run,
including startup. After the first interval, every measured 60-VBlank interval
completed exactly 60 renders. Music continued; camera/player bounds still reached
1120/1398. Real hardware, PAL and other cores remain unverified.

## Portal Hub extensions

`GdlTilemap.tile_rows` optionally points to a compact foreground row directory.
When present, map values index that directory, with eight offsets per tile;
each directory value selects the usual five-word packed foreground row. NULL
retains the original fixed-stride layout. Duplicate rows can share storage.

`GdlSprite.behind_foreground` enables terrain occlusion for environmental
sprites. Foreground transparency masks provide coverage; sprites without the
flag retain the original frontmost behavior. The direct renderer remains a
separate optimized path for scenes that need neither extension.

`gdl_palette_schedule` copies colors into a pending palette and applies them
at the next completed flip. It preserves gaps between queued ranges from the
backend's palette shadow. `gdl_palette_load` is a blocking initial load and
waits for palette access. Resource packing now also supports deduplicating
identical opaque runs without changing the span decoder.

T_Air_1 uses `GdlFrame.background_rows[28]` with `background_per_row` enabled
for independent X offsets per visible 8px band. The entire table is copied
with the frame snapshot before either SH-2 draws. Background Y stays separate.
An optional LSB-first `foreground_priority` map bitset lets sprites with
`behind_foreground=2` pass behind priority tiles only; value 1 retains full
foreground occlusion and value 0 keeps sprites above terrain. Priority maps
must remain immutable for the active frame.

Water 1 uses `background_per_line` and all 224 `background_rows` entries for
world-height-based raster drift. An optional `background_priority` map enables
two-plane composition in `renderer_planes.c`; high background pixels can cover
low foreground and low-priority sprites. Transparent palette-bank color zero
does not occlude sprites. Bands containing only low-priority background tiles
use the existing compositor. Compact background `tile_rows` directories index
unique two-word rows and keep the larger background within SDRAM limits.

`background_opaque_rows` optionally marks source pixel rows guaranteed to be
both high-priority and opaque across their full width. Those rows use a small
compositor that skips hidden low-priority sprites and low foreground. Source
conversion verifies the guarantee; mixed/transparent rows retain the general
priority compositor. The priority compositor uses the SGDK-derived 16-color
bank convention, where a palette bank's color zero does not occlude sprites.


Level changes bind scenes only while `gdl_render_frame` is idle. Fully uploaded
arrays marked `GDL_LEVEL_ASSETS(level)` share NOBITS linker overlays in SDRAM;
never retain active references to a prior level's assets after loading another.
The overlay is cleared at boot and must be fully replaced by each level loader.
`gdl_scheduler_init` invalidates the master cache and publishes a new scene
generation; the slave invalidates its cache before drawing that generation.
Persistent player, level, and frame state remain outside the overlaid sections.


The current frame budget is 16 visible sprites. Hub submission reserves its
last slot for Chakan and clips scenery before consuming slots. Portal transition
fades capture the first 80 colors, including the separate 16-color planet bank.


Water's optional `background_pixel_priority` format stores opaque/high coverage
in background pixel bit 7 (colors 0..127). Its `foreground_map_priority` format
stores priority in compact foreground map bit 15 (directory indices 0..32767).
The bitsets remain available for band dispatch and legacy priority paths.
Both flags require resources converted for that encoding; ordinary resources
leave them false. The background aligner uses fixed shifts for SH-2.
Water jobs publish a 128/96 row split; other jobs retain 112/112. The slave frame
snapshot uses dedicated SDRAM to keep nested scanline buffers within its stack.
