#ifndef GDL_RENDERER_H
#define GDL_RENDERER_H
#include <stdbool.h>
#include <stdint.h>

#define GDL_WIDTH 320
#define GDL_HEIGHT 224
#define GDL_MAX_SPRITES 16

/* Version 1 packed rows, containing final 8-bit palette indices.
 * Foreground row: pixels[2], transparent masks[2], kind (0 empty/1 opaque/2 mixed).
 * Background row: pixels[2]. Map entries are uint16_t offsets measured in uint32_t words.
 * Compact maps instead index tile_rows (eight row offsets per tile).
 * Tiles are 8x8; foreground tile stride is 40 words, background stride 16 words.
 */
typedef struct {
    const uint32_t *tiles;
    const uint16_t *map;
    uint16_t width_tiles, height_tiles;
    const uint16_t *tile_rows; /* Optional compact row offsets; NULL for fixed stride. */
} GdlTilemap;

typedef struct {
    /* row_offsets[2 * frames * height + 1] covers normal then flipped frames.
     * Each span is three uint16_t values: x, count, offset into pixels.
     * Pixels contain final palette indices and omit transparent runs. */
    const uint16_t *row_offsets, *spans;
    const uint8_t *pixels;
    uint16_t width, height, frames;
} GdlSpanImage;

enum {
    GDL_SPRITE_ABOVE_FOREGROUND = 0,
    GDL_SPRITE_BEHIND_FOREGROUND = 1,
    GDL_SPRITE_BEHIND_PRIORITY = 2
};
typedef struct {
    const GdlSpanImage *image;
    int16_t x, y; /* Viewport coordinates; clipping is performed by the renderer. */
    uint16_t frame;
    bool flip;
    uint8_t behind_foreground; /* 0: above terrain, 1: behind all, 2: behind priority tiles. */
} GdlSprite;

typedef struct {
    GdlTilemap foreground, background;
} GdlScene;

/* Prepared 320x224 screen with sparse text words and an optional animation
 * rectangle. Used by fixed-camera cinematics; camera_y selects text rows. */
typedef struct {
    const uint32_t *background, *patches, *animation;
    const uint16_t *patch_rows;
    uint16_t animation_x_words, animation_y, animation_width_words, animation_height;
} GdlScreen;

typedef struct {
    int16_t camera_x, camera_y;
    uint16_t background_x, background_y;
    bool background_per_row;
    bool background_per_line;
    bool background_pixel_priority; /* BG byte bit 7 encodes opaque/high priority; colors use bits 0..6. */
    bool foreground_map_priority; /* FG compact map bit 15 encodes tile priority. */
    uint16_t background_rows[GDL_HEIGHT]; /* X per scanline, or first 28 entries per 8px band. */
    const uint8_t *foreground_priority; /* Optional map bitset, LSB first per tile. */
    const uint8_t *background_priority; /* Enables full two-plane priority composition. */
    const uint8_t *background_opaque_rows; /* Optional per-background-pixel-row high/opaque guarantee. */
    const GdlScreen *screen; /* Optional prepared cinematic compositor. */
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
 * The background repeats in both axes using independent X/Y offsets.
 * Its width must be a power of two in tiles; its height may be any positive size.
 * Rows [first,end) must be exclusive to this caller until rendering completes. */
void gdl_render_rows(const GdlScene *scene, const GdlFrame *frame,
                     const GdlSurface *surface, uint16_t first, uint16_t end);
#endif
