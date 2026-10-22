#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include "gdl/renderer.h"

#define PITCH 83
#define GUARD 0xDEADBEEFUL
static uint32_t output[4 + GDL_HEIGHT * PITCH + 4];
static uint32_t fg_tiles[3 * 40], bg_tiles[16 * 3];
static uint16_t fg_map[48 * 30], bg_map[3];
static uint16_t rows[13], spans[200];
static uint8_t pixels[100];
static GdlSpanImage image = {rows, spans, pixels, 5, 3, 2};

static uint8_t foreground(int tile, int x, int y) {
    if (tile == 0 || (tile == 2 && ((x + y) & 1))) return 0;
    return 40 + ((x + y) & 7);
}
static uint8_t background(int x, int y) { return (y&1) && x==3 ? 96 : 97 + (x * 3 + y * 5) % 15; }
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
                bg_tiles[tile * 16 + y * 2 + x / 4] |= (uint32_t)(background(x, y) + tile) << shift;
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
    unsigned split=frame->background_pixel_priority?128:112;
    gdl_render_rows(scene, frame, &surface, 0, split);
    assert(output[4 + split * PITCH] == GUARD);
    gdl_render_rows(scene, frame, &surface, split, 300); /* End is clipped to 224. */
    for (int y = 0; y < GDL_HEIGHT; y++) {
        for (int x = 0; x < GDL_WIDTH; x++) {
            int wx = x + frame->camera_x, wy = y + frame->camera_y;
            int tile = ((wx / 8) + (wy / 8)) % 3;
            uint8_t expected = foreground(tile, wx & 7, wy & 7);
            int terrain_opaque = expected != 0;
            int bx = x + (frame->background_per_line ? frame->background_rows[y] : frame->background_per_row ? frame->background_rows[y/8] : frame->background_x);
            int by = (y + frame->background_y) % (scene->background.height_tiles * 8);
            int bg_tile=(by/8)*scene->background.width_tiles + ((bx/8)%scene->background.width_tiles);
            uint8_t bg = background(bx&7,by&7) + scene->background.map[bg_tile]/(scene->background.tile_rows?8:16);
            int index=(wy/8)*scene->foreground.width_tiles + wx/8;
            int priority=frame->foreground_priority && (frame->foreground_priority[index/8] & (1u<<(index&7)));
            int bg_high=frame->background_priority && (frame->background_priority[bg_tile/8] & (1u<<(bg_tile&7))) && (bg&15);
            if (!expected || (bg_high && !priority)) expected=bg;
            for (int i = 0; i < frame->sprite_count; i++) {
                const GdlSprite *sprite = frame->sprites + i;
                int sx = x - sprite->x, sy = y - sprite->y;
                if ((sprite->behind_foreground==1 && (terrain_opaque || bg_high)) ||
                    (sprite->behind_foreground==2 && ((terrain_opaque && priority) || bg_high))) continue;
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
            frame.background_x = camera / 2;
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
    scene.background.width_tiles=2; bg_map[1]=16;
    /* Every foreground/background sub-tile alignment, including repeat wrap. */
    for (int fg=0; fg<8; fg++) for (int bg=0; bg<16; bg++) {
        frame.camera_x=fg; frame.background_x=bg;
        check_frame(&scene,&frame);
    }
    /* Non-power-of-two background height, including its repeat boundary. */
    scene.background.width_tiles=1;
    scene.background.height_tiles = 3;
    bg_map[1] = 16; bg_map[2] = 32;
    frame.background_y = 23;
    check_frame(&scene, &frame);
    scene.foreground.map = fg_map;
    scene.foreground.tile_rows = 0;
    frame.sprites[0].behind_foreground = false;
    check_frame(&scene, &frame);
    for (int fg=0; fg<8; fg++) for (int bg=0; bg<16; bg++) {
        frame.camera_x=fg; frame.background_x=bg;
        check_frame(&scene,&frame);
    }
    scene.background.width_tiles=2; scene.background.height_tiles=1;
    for (int fg=0; fg<8; fg++) for (int bg=0; bg<32; bg++) {
        frame.camera_x=fg; frame.background_x=bg;
        check_frame(&scene,&frame);
    }
    /* Per-visible-row scroll and foreground priority are snapshotted per frame. */
    uint8_t priority_bits[48*30/8];
    for (unsigned i=0;i<sizeof(priority_bits);i++) priority_bits[i]=0x55;
    frame.foreground_priority=priority_bits;
    frame.background_per_row=true;
    for (int row=0;row<GDL_HEIGHT/8;row++) frame.background_rows[row]=row*7;
    frame.sprites[0].behind_foreground=2;
    frame.sprites[1].behind_foreground=1;
    check_frame(&scene,&frame);
    scene.foreground.map=compact_map; scene.foreground.tile_rows=compact_rows;
    frame.sprites[1].behind_foreground=2;
    check_frame(&scene,&frame);
    frame.foreground_priority=0;
    check_frame(&scene,&frame);
    /* Water: per-scanline scroll, high BG over low FG, mixed sprite depths. */
    uint8_t bg_priority=1;
    frame.background_priority=&bg_priority;
    frame.foreground_priority=priority_bits;
    frame.background_per_line=true;
    frame.background_per_row=false;
    for (int y=0;y<GDL_HEIGHT;y++) frame.background_rows[y]=y*13;
    frame.sprites[0].behind_foreground=2; frame.sprites[1].behind_foreground=0;
    for (int camera=0;camera<8;camera++) {
        frame.camera_x=camera;
        bg_priority=1; check_frame(&scene,&frame);
        bg_priority=2; check_frame(&scene,&frame);
    }
    uint16_t compact_bg_map[2]={0,8}, compact_bg_rows[24];
    for (int i=0;i<24;i++) compact_bg_rows[i]=i*2;
    scene.background.map=compact_bg_map; scene.background.tile_rows=compact_bg_rows;
    frame.sprites[1].behind_foreground=1;
    check_frame(&scene,&frame);
    frame.foreground_priority=0;
    check_frame(&scene,&frame);
    /* Opaque high BG rows bypass hidden low-priority sprites and low FG. */
    uint8_t opaque_rows[8]={1,0,1,0,1,0,1,0};
    scene.background.width_tiles=1;
    frame.background_priority=&bg_priority; bg_priority=1;
    frame.background_opaque_rows=opaque_rows;
    frame.foreground_priority=priority_bits;
    frame.sprites[0].behind_foreground=2; frame.sprites[1].behind_foreground=0;
    for (int x=0;x<8;x++) { frame.camera_x=x; check_frame(&scene,&frame); }
    frame.sprites[1].behind_foreground=1;
    check_frame(&scene,&frame);
    /* Baked per-pixel high/opaque bits must match the general tile-priority
     * compositor, including sub-tile alignment and transparent color zero. */
    uint32_t tagged_bg[48];
    for (int word=0;word<48;word++) {
        tagged_bg[word]=bg_tiles[word];
        if (word/16==0) for (int shift=0;shift<32;shift+=8)
            if ((bg_tiles[word]>>shift)&15) tagged_bg[word]|=128UL<<shift;
    }
    scene.background.tiles=tagged_bg;
    frame.background_pixel_priority=true;
    uint16_t tagged_fg[48*30];
    for (int i=0;i<48*30;i++) tagged_fg[i]=compact_map[i]|
        ((priority_bits[i>>3]&(1<<(i&7)))?32768:0);
    scene.foreground.map=tagged_fg;
    frame.foreground_map_priority=true;
    scene.background.width_tiles=2;
    frame.background_opaque_rows=0;
    for (int x=0;x<8;x++) { frame.camera_x=x; check_frame(&scene,&frame); }
    scene.background.width_tiles=1;
    frame.background_opaque_rows=opaque_rows;
    for (int x=0;x<8;x++) { frame.camera_x=x; check_frame(&scene,&frame); }
    scene.background.tiles=bg_tiles;
    frame.background_pixel_priority=false;
    scene.foreground.map=compact_map;
    frame.foreground_map_priority=false;
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
