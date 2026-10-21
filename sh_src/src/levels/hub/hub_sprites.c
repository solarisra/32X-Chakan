#include "hub_sprites.h"
#include "player_sprite.h"
#include "portal_hub_data.h"
static uint16_t character_rows[sizeof(hub_chakan_rows) / 2], character_spans[sizeof(hub_chakan_spans) / 2];
static uint8_t character_pixels[sizeof(hub_chakan_pixels)];
static uint16_t flame_rows[sizeof(hub_flame_rows) / 2], flame_spans[sizeof(hub_flame_spans) / 2];
static uint8_t flame_pixels[sizeof(hub_flame_pixels)];
static const GdlSpanImage chakan = {character_rows, character_spans, character_pixels, 112, 80, 12};
static const GdlSpanImage flame = {flame_rows, flame_spans, flame_pixels, 16, 32, 6};
#define PORTAL_STORAGE(name) \
    static uint16_t name##_rows[sizeof(hub_##name##_rows) / 2]; \
    static uint16_t name##_spans[sizeof(hub_##name##_spans) / 2]; \
    static uint8_t name##_pixels[sizeof(hub_##name##_pixels)];
PORTAL_STORAGE(earth)
PORTAL_STORAGE(fire)
PORTAL_STORAGE(water)
PORTAL_STORAGE(air)
#define PORTAL(name) {name##_rows, name##_spans, name##_pixels, 64, 88, 1}
static const GdlSpanImage portals[] = {PORTAL(earth), PORTAL(fire), PORTAL(water), PORTAL(air)};
static const int16_t portal_positions[][2] = {{384,175}, {1696,335}, {576,1135}, {1375,1103}};
static const int16_t flame_positions[][2] = {{135,146}, {839,338}, {103,562}, {1127,562}, {103,914}, {1351,882}, {1575,882}, {807,1042}, {999,1042}};
void hub_sprites_init(void) {
    gdl_shared_upload(character_rows, hub_chakan_rows, sizeof(character_rows));
    gdl_shared_upload(character_spans, hub_chakan_spans, sizeof(character_spans));
    gdl_shared_upload(character_pixels, hub_chakan_pixels, sizeof(character_pixels));
    gdl_shared_upload(flame_rows, hub_flame_rows, sizeof(flame_rows));
    gdl_shared_upload(flame_spans, hub_flame_spans, sizeof(flame_spans));
    gdl_shared_upload(flame_pixels, hub_flame_pixels, sizeof(flame_pixels));
#define UPLOAD_PORTAL(name) \
    gdl_shared_upload(name##_rows, hub_##name##_rows, sizeof(name##_rows)); \
    gdl_shared_upload(name##_spans, hub_##name##_spans, sizeof(name##_spans)); \
    gdl_shared_upload(name##_pixels, hub_##name##_pixels, sizeof(name##_pixels));
    UPLOAD_PORTAL(earth)
    UPLOAD_PORTAL(fire)
    UPLOAD_PORTAL(water)
    UPLOAD_PORTAL(air)
    player_sprite_init();
}
static void submit(GdlFrame *frame, const GdlSpanImage *image, int16_t x, int16_t y, uint16_t animation_frame) {
    x -= frame->camera_x; y -= frame->camera_y;
    if (x + image->width <= 0 || x >= GDL_WIDTH || y + image->height <= 0 || y >= GDL_HEIGHT) return;
    if (frame->sprite_count >= GDL_MAX_SPRITES) return;
    GdlSprite *sprite = &frame->sprites[frame->sprite_count++];
    *sprite = (GdlSprite){image, x, y, animation_frame, false, true};
}
void hub_sprites_submit(GdlFrame *frame, uint32_t tick) {
    frame->sprite_count = 0;
    for (uint16_t i = 0; i < 4; i++) submit(frame, &portals[i], portal_positions[i][0], portal_positions[i][1], 0);
    for (uint16_t i = 0; i < 9; i++) submit(frame, &flame, flame_positions[i][0], flame_positions[i][1], (tick / 8) % 6);
    player_sprite_submit(frame, &chakan, frame->camera_x, frame->camera_y);
}
