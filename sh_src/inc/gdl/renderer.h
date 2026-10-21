#ifndef GDL_RENDERER_H
#define GDL_RENDERER_H
#include <stdbool.h>
#include <stdint.h>

#define GDL_WIDTH 320
#define GDL_HEIGHT 224
#define GDL_MAX_SPRITES 8

/* Version 1 packed rows, containing final 8-bit palette indices.
 * Foreground row: pixels[2], transparent masks[2], kind (0 empty/1 opaque/2 mixed).
 * Background row: pixels[2]. Map entries are uint16_t offsets measured in uint32_t words.
 * Tiles are 8x8; foreground tile stride is 40 words, background stride 16 words.
 */
typedef struct {
    const uint32_t *tiles;
    const uint16_t *map;
    uint16_t width_tiles, height_tiles;
} GdlTilemap;

typedef struct {
    /* row_offsets[2 * frames * height + 1] covers normal then flipped frames.
     * Each span is three uint16_t values: x, count, offset into pixels.
     * Pixels contain final palette indices and omit transparent runs. */
    const uint16_t *row_offsets, *spans;
    const uint8_t *pixels;
    uint16_t width, height, frames;
} GdlSpanImage;

typedef struct {
    const GdlSpanImage *image;
    int16_t x, y; /* Viewport coordinates; clipping is performed by the renderer. */
    uint16_t frame;
    bool flip;
} GdlSprite;

typedef struct {
    GdlTilemap foreground, background;
} GdlScene;

typedef struct {
    int16_t camera_x, camera_y;
    uint16_t background_y;
    uint16_t sprite_count;
    GdlSprite sprites[GDL_MAX_SPRITES]; /* Back-to-front order. */
} GdlFrame;

typedef struct {
    volatile uint32_t *pixels; /* First pixel, excluding any hardware line table. */
    uint16_t pitch_words;     /* In 32-bit words; at least 80. */
} GdlSurface;

bool gdl_scene_valid(const GdlScene *scene);
bool gdl_frame_valid(const GdlScene *scene, const GdlFrame *frame);
/* Pure rasterizer: no hardware registers, allocation, gameplay or scheduling.
 * The background repeats in both axes and shares camera X with the foreground.
 * Rows [first,end) must be exclusive to this caller until rendering completes. */
void gdl_render_rows(const GdlScene *scene, const GdlFrame *frame,
                     const GdlSurface *surface, uint16_t first, uint16_t end);
#endif
