#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include "gdl/renderer.h"

#define PITCH 83
#define GUARD 0xDEADBEEFUL
static uint32_t output[4 + GDL_HEIGHT * PITCH + 4];
static uint32_t fg_tiles[3 * 40], bg_tiles[16];
static uint16_t fg_map[48 * 30], bg_map[1];
static uint16_t rows[13], spans[200];
static uint8_t pixels[100];
static GdlSpanImage image = {rows, spans, pixels, 5, 3, 2};

static uint8_t foreground(int tile, int x, int y) {
    if (tile == 0 || (tile == 2 && ((x + y) & 1))) return 0;
    return 40 + ((x + y) & 7);
}
static uint8_t background(int x, int y) { return 100 + (x * 3 + y * 5) % 15; }
static uint8_t sprite_pixel(int frame, int flip, int x, int y) {
    if (flip) x = 4 - x;
    return x == 2 ? 0 : 160 + frame * 20 + y * 5 + x;
}
static void prepare(void) {
    for (int tile = 0; tile < 3; tile++) {
        for (int y = 0; y < 8; y++) {
            uint32_t *row = fg_tiles + tile * 40 + y * 5;
            int count = 0;
            for (int x = 0; x < 8; x++) {
                uint8_t pixel = foreground(tile, x, y);
                int shift = (3 - (x & 3)) * 8;
                row[x / 4] |= (uint32_t)pixel << shift;
                row[2 + x / 4] |= (uint32_t)(pixel ? 0 : 255) << shift;
                count += pixel != 0;
                if (tile == 0) bg_tiles[y * 2 + x / 4] |= (uint32_t)background(x, y) << shift;
            }
            row[4] = count == 0 ? 0 : count == 8 ? 1 : 2;
        }
    }
    for (int y = 0; y < 30; y++)
        for (int x = 0; x < 48; x++) fg_map[y * 48 + x] = ((x + y) % 3) * 40;
    unsigned row_index = 0, span_index = 0, data_index = 0;
    for (int flip = 0; flip < 2; flip++)
        for (int frame = 0; frame < 2; frame++)
            for (int y = 0; y < 3; y++) {
                rows[row_index++] = span_index;
                for (int x = 0; x < 5;) {
                    if (!sprite_pixel(frame, flip, x, y)) { x++; continue; }
                    int start = x;
                    unsigned offset = data_index;
                    while (x < 5 && sprite_pixel(frame, flip, x, y))
                        pixels[data_index++] = sprite_pixel(frame, flip, x++, y);
                    spans[span_index * 3] = start;
                    spans[span_index * 3 + 1] = x - start;
                    spans[span_index * 3 + 2] = offset;
                    span_index++;
                }
            }
    rows[row_index] = span_index;
}
static void check_frame(const GdlScene *scene, const GdlFrame *frame) {
    GdlSurface surface = {output + 4, PITCH};
    for (unsigned i = 0; i < sizeof(output) / sizeof(output[0]); i++) output[i] = GUARD;
    assert(gdl_frame_valid(scene, frame));
    gdl_render_rows(scene, frame, &surface, 0, 112);
    assert(output[4 + 112 * PITCH] == GUARD);
    gdl_render_rows(scene, frame, &surface, 112, 300); /* End is clipped to 224. */
    for (int y = 0; y < GDL_HEIGHT; y++) {
        for (int x = 0; x < GDL_WIDTH; x++) {
            int wx = x + frame->camera_x, wy = y + frame->camera_y;
            int tile = ((wx / 8) + (wy / 8)) % 3;
            uint8_t expected = foreground(tile, wx & 7, wy & 7);
            int terrain_opaque = expected != 0;
            if (!expected) expected = background(wx & 7, (y + frame->background_y) & 7);
            for (int i = 0; i < frame->sprite_count; i++) {
                const GdlSprite *sprite = frame->sprites + i;
                int sx = x - sprite->x, sy = y - sprite->y;
                if (sprite->behind_foreground && terrain_opaque) continue;
                if (sx >= 0 && sx < 5 && sy >= 0 && sy < 3) {
                    uint8_t pixel = sprite_pixel(sprite->frame, sprite->flip, sx, sy);
                    if (pixel) expected = pixel;
                }
            }
            uint32_t word = output[4 + y * PITCH + x / 4];
            assert(((word >> ((3 - (x & 3)) * 8)) & 255) == expected);
        }
        for (int x = 80; x < PITCH; x++) assert(output[4 + y * PITCH + x] == GUARD);
    }
    for (int i = 0; i < 4; i++) {
        assert(output[i] == GUARD);
        assert(output[4 + GDL_HEIGHT * PITCH + i] == GUARD);
    }
}
int main(void) {
    prepare();
    GdlScene scene = {{fg_tiles, fg_map, 48, 30, 0}, {bg_tiles, bg_map, 1, 1, 0}};
    assert(gdl_scene_valid(&scene));
    GdlFrame frame = {0};
    frame.background_y = 5;
    check_frame(&scene, &frame);
    int positions[] = {SHRT_MIN, -20, -4, -1, 0, 2, 310, 318, 320, SHRT_MAX};
    frame.sprite_count = 2;
    for (int camera = 0; camera < 8; camera++)
        for (unsigned p = 0; p < sizeof(positions) / sizeof(positions[0]); p++) {
            frame.camera_x = camera;
            frame.camera_y = (p & 1) ? 16 : 0;
            frame.sprites[0] = (GdlSprite){&image, positions[p], -1, 0, false, false};
            frame.sprites[1] = (GdlSprite){&image, 1, 0, 1, true, false};
            frame.sprites[0].behind_foreground = (p & 1) != 0;
            check_frame(&scene, &frame);
        }
    frame.sprites[0].y = 222; /* Bottom clipping and overlapping sprites. */
    frame.sprites[0].x = 2;
    frame.sprites[1].y = 223;
    check_frame(&scene, &frame);
    /* The optional compact row directory must produce the same pixels. */
    uint16_t compact_map[48 * 30], compact_rows[3 * 8];
    for (int i = 0; i < 48 * 30; i++) compact_map[i] = fg_map[i] / 5;
    for (int i = 0; i < 3 * 8; i++) compact_rows[i] = i * 5;
    scene.foreground.map = compact_map;
    scene.foreground.tile_rows = compact_rows;
    for (int camera = 0; camera < 8; camera++) {
        frame.camera_x = camera;
        frame.sprites[0].y = 0;
        frame.sprites[0].behind_foreground = true;
        check_frame(&scene, &frame);
    }
    frame.camera_x = -1;
    assert(!gdl_frame_valid(&scene, &frame));
    frame.camera_x = 65; /* 384px world, 320px viewport. */
    assert(!gdl_frame_valid(&scene, &frame));
    frame.camera_x = 0;
    frame.sprites[0].frame = 2;
    assert(!gdl_frame_valid(&scene, &frame));
    frame.sprites[0].frame = 0;
    frame.sprite_count = GDL_MAX_SPRITES + 1;
    assert(!gdl_frame_valid(&scene, &frame));
    scene.background.width_tiles = 3;
    assert(!gdl_scene_valid(&scene));
    puts("GDL renderer pixel, clipping, depth, alignment and pitch checks passed");
    return 0;
}
