#include "gdl/renderer.h"
#include <stddef.h>

void gdl_render_rows_direct(const GdlScene *, const GdlFrame *, const GdlSurface *, uint16_t, uint16_t);

static bool power_of_two(uint16_t value) { return value && !(value & (value - 1)); }
bool gdl_scene_valid(const GdlScene *scene) {
    return scene && scene->foreground.tiles && scene->foreground.map
        && scene->background.tiles && scene->background.map
        && scene->foreground.width_tiles >= GDL_WIDTH / 8
        && scene->foreground.height_tiles >= GDL_HEIGHT / 8
        && scene->foreground.width_tiles <= 4095 && scene->foreground.height_tiles <= 4095
        && power_of_two(scene->background.width_tiles)
        && power_of_two(scene->background.height_tiles)
        && scene->background.width_tiles <= 4096 && scene->background.height_tiles <= 4096;
}
bool gdl_frame_valid(const GdlScene *scene, const GdlFrame *frame) {
    if (!gdl_scene_valid(scene) || !frame || frame->sprite_count > GDL_MAX_SPRITES
        || frame->camera_x < 0 || frame->camera_y < 0
        || frame->camera_x > scene->foreground.width_tiles * 8 - GDL_WIDTH
        || frame->camera_y > scene->foreground.height_tiles * 8 - GDL_HEIGHT) return false;
    for (uint16_t i = 0; i < frame->sprite_count; i++) {
        const GdlSprite *sprite = &frame->sprites[i];
        const GdlSpanImage *image = sprite->image;
        if (!image || !image->row_offsets || !image->spans || !image->pixels
            || !image->width || !image->height || image->width > 32767
            || image->height > 32767 || sprite->frame >= image->frames) return false;
    }
    return true;
}

/* Both processors draw disjoint scanlines of the same hidden framebuffer. */
static inline __attribute__((always_inline)) void render_core(const GdlScene *scene, const GdlFrame *frame,
                     const GdlSurface *surface, uint16_t first, uint16_t end, bool compact, bool allow_occlusion) {
    int16_t camera_x = frame->camera_x, camera_y = frame->camera_y;
    uint16_t star_y = frame->background_y;
    const uint32_t *fg_tiles = scene->foreground.tiles, *bg_tiles = scene->background.tiles;
    const uint16_t *fg_row_offsets = scene->foreground.tile_rows;
    const uint16_t *fg_map = scene->foreground.map, *bg_map = scene->background.map;
    uint16_t fg_width = scene->foreground.width_tiles;
    uint16_t bg_width = scene->background.width_tiles;
    uint16_t bg_x_mask = bg_width - 1;
    uint16_t bg_y_mask = scene->background.height_tiles * 8 - 1;
    if (!surface || !surface->pixels || surface->pitch_words < GDL_WIDTH / 4) return;
    if (first > GDL_HEIGHT) first = GDL_HEIGHT;
    if (end > GDL_HEIGHT) end = GDL_HEIGHT;
    for (uint16_t y = first; y < end; y++) {
        uint32_t words[82];
        uint32_t coverage[82];
        bool occlusion = false;
        if (allow_occlusion) for (uint16_t si = 0; si < frame->sprite_count; si++) {
            const GdlSprite *sprite = &frame->sprites[si];
            int32_t sy = (int32_t)y - sprite->y;
            if (sprite->behind_foreground && sy >= 0 && sy < sprite->image->height) { occlusion = true; break; }
        }
#if __BYTE_ORDER__ != __ORDER_LITTLE_ENDIAN__
        uint8_t *line = (uint8_t *)words;
#endif
        uint16_t fy = y + camera_y, by = (y + star_y) & bg_y_mask;
        const uint16_t *fm = fg_map + (fy >> 3) * fg_width;
        const uint16_t *bm = bg_map + (by >> 3) * bg_width;
        uint16_t offset = camera_x & 7;
        uint16_t tile_x = camera_x >> 3;
        uint16_t columns = (320 + offset + 7) >> 3;
        for (uint16_t column = 0; column < columns; column++, tile_x++) {
            const uint32_t *fg = fg_tiles + (compact
                ? fg_row_offsets[fm[tile_x] + (fy & 7)]
                : fm[tile_x] + (fy & 7) * 5);
            const uint32_t *bg = bg_tiles + bm[tile_x & bg_x_mask] + (by & 7) * 2;
            uint32_t *output = words + column * 2;
            if (allow_occlusion && occlusion) { coverage[column * 2] = fg[2]; coverage[column * 2 + 1] = fg[3]; }
            if (fg[4] == 0) {
                output[0] = bg[0];
                output[1] = bg[1];
            } else if (fg[4] == 1) {
                output[0] = fg[0];
                output[1] = fg[1];
            } else {
                output[0] = fg[0] | (bg[0] & fg[2]);
                output[1] = fg[1] | (bg[1] & fg[3]);
            }
        }
        for (uint16_t si = 0; si < frame->sprite_count; si++) {
            const GdlSprite *sprite = &frame->sprites[si];
            const GdlSpanImage *image = sprite->image;
            int32_t sy = (int32_t)y - sprite->y;
            if (sy < 0 || sy >= image->height) continue;
            uint32_t row_index = (sprite->frame + (sprite->flip ? image->frames : 0)) * image->height + sy;
            for (uint16_t span = image->row_offsets[row_index]; span < image->row_offsets[row_index + 1]; span++) {
                const uint16_t *run = image->spans + span * 3;
                int32_t left = (int32_t)sprite->x + run[0];
                int32_t right = left + run[1];
                if (right <= 0 || left >= GDL_WIDTH) continue;
                const uint8_t *pixels = image->pixels + run[2];
                if (left < 0) { pixels -= left; left = 0; }
                if (right > GDL_WIDTH) right = GDL_WIDTH;
                for (int16_t x = left; x < right; x++) {
                    if (allow_occlusion && sprite->behind_foreground) {
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
                        uint16_t pixel = x + offset;
                        if (!((coverage[pixel >> 2] >> ((3 - (pixel & 3)) * 8)) & 255)) { pixels++; continue; }
#else
                        if (!((const uint8_t *)coverage)[x + offset]) { pixels++; continue; }
#endif
                    }
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
                    /* Host tests use the same canonical big-endian pixel words. */
                    uint16_t pixel = x + offset;
                    uint16_t shift = (3 - (pixel & 3)) * 8;
                    words[pixel >> 2] = (words[pixel >> 2] & ~(255UL << shift)) | ((uint32_t)*pixels++ << shift);
#else
                    line[x + offset] = *pixels++;
#endif
                }
            }
        }
        const uint32_t *source = words + (offset >> 2);
        volatile uint32_t *destination = surface->pixels + y * surface->pitch_words;
        switch (offset & 3) {
        case 0:
            #pragma GCC unroll 8
            for (uint16_t x = 0; x < 80; x++) destination[x] = source[x];
            break;
        case 1:
            #pragma GCC unroll 8
            for (uint16_t x = 0; x < 80; x++)
                destination[x] = (source[x] << 8) | (source[x + 1] >> 24);
            break;
        case 2:
            #pragma GCC unroll 8
            for (uint16_t x = 0; x < 80; x++)
                destination[x] = (source[x] << 16) | (source[x + 1] >> 16);
            break;
        case 3:
            #pragma GCC unroll 8
            for (uint16_t x = 0; x < 80; x++)
                destination[x] = (source[x] << 24) | (source[x + 1] >> 8);
            break;
        }
    }
}


void gdl_render_rows(const GdlScene *scene, const GdlFrame *frame,
                     const GdlSurface *surface, uint16_t first, uint16_t end) {
    bool behind = false;
    for (uint16_t i = 0; i < frame->sprite_count; i++) if (frame->sprites[i].behind_foreground) { behind = true; break; }
    if (scene->foreground.tile_rows) {
        if (behind) render_core(scene, frame, surface, first, end, true, true);
        else render_core(scene, frame, surface, first, end, true, false);
    } else {
        if (behind) render_core(scene, frame, surface, first, end, false, true);
        else gdl_render_rows_direct(scene, frame, surface, first, end);
    }
}
