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
- One finite foreground and one repeating opaque background; both share camera
  X. Background Y is independent. Background tile dimensions must be powers of
  two. The application clamps foreground camera bounds before submission.
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
