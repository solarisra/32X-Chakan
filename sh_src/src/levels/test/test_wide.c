#include "test_wide.h"
#include "player.h"
#include "input_controls.h"
#include "camera.h"
#include "test_wide_data.h"

static GdlFrame render_frame;
static uint16_t star_drift, animation, animation_frame, animation_ticks;
/* Tile data stays in SDRAM so the two renderers do not compete for ROM. */
static uint32_t fg_tiles[sizeof(wide_fg_tiles) / 4], bg_tiles[sizeof(wide_bg_tiles) / 4];
static uint16_t fg_map[sizeof(wide_fg_map) / 2], bg_map[sizeof(wide_bg_map) / 2];
static uint8_t sprite_pixels[sizeof(wide_sprite_pixels)];
static uint16_t sprite_rows[sizeof(wide_sprite_rows) / 2];
static uint16_t sprite_spans[sizeof(wide_sprite_spans) / 2];
static uint8_t collision[sizeof(wide_collision)];
static const GdlSpanImage chakan_image = {sprite_rows, sprite_spans, sprite_pixels, 112, 80, 12};
static const GdlScene render_scene = {
    {fg_tiles, fg_map, 180, 30}, {bg_tiles, bg_map, 64, 32}
};

static bool blocked(int16_t x, int16_t y) {
    if (x < 0 || x + PLAYER_WIDTH > 1440 || y < 0) return true;
    for (int16_t ty = y / 16; ty <= (y + PLAYER_HEIGHT - 1) / 16 && ty < 15; ty++)
        for (int16_t tx = x / 16; tx <= (x + PLAYER_WIDTH - 1) / 16; tx++)
            if (collision[ty * 90 + tx]) return true;
    return false;
}

void test_wide_init(void) {
    gdl_shared_upload(fg_tiles, wide_fg_tiles, sizeof(fg_tiles));
    gdl_shared_upload(bg_tiles, wide_bg_tiles, sizeof(bg_tiles));
    gdl_shared_upload(sprite_pixels, wide_sprite_pixels, sizeof(sprite_pixels));
    gdl_shared_upload(sprite_rows, wide_sprite_rows, sizeof(sprite_rows));
    gdl_shared_upload(sprite_spans, wide_sprite_spans, sizeof(sprite_spans));
    gdl_shared_upload(fg_map, wide_fg_map, sizeof(fg_map));
    gdl_shared_upload(bg_map, wide_bg_map, sizeof(bg_map));
    for (uint32_t i = 0; i < sizeof(collision); i++) collision[i] = wide_collision[i];
    static const uint16_t black = 0;
    gdl_palette_load(0, wide_palette, 48);
    gdl_palette_load(0, &black, 1);
    gdl_palette_load(16, &black, 1);
    gdl_palette_load(32, &black, 1);
    gdl_scheduler_init(&render_scene);
    render_frame.sprite_count = 1;
    render_frame.sprites[0].image = &chakan_image;
    playerInit(WIDE_SPAWN_X + PLAYER_OFFSET_X, WIDE_SPAWN_Y + PLAYER_OFFSET_Y, blocked);
    input_init();
    camera_init(1440, 240);
    star_drift = animation = animation_frame = animation_ticks = 0;
}

void test_wide_update(uint16_t pad) {
    processInput(pad);
    updatePlayer();
    if (playerBody.y >= 240)
        playerInit(WIDE_SPAWN_X + PLAYER_OFFSET_X, WIDE_SPAWN_Y + PLAYER_OFFSET_Y, blocked);
    camera_step(playerBody.x + PLAYER_WIDTH / 2, playerBody.y + PLAYER_HEIGHT / 2);
    star_drift = (star_drift + 1) & 511;
    uint16_t next = playerBody.onGround ? (playerBody.velocity_x ? 1 : 0)
        : (playerBody.fixY < 0 ? 2 : 3);
    if (playerBody.onGround && animation == 3) next = 4;
    if (animation == 4 && playerBody.onGround && animation_ticks < 8) next = 4;
    if (next != animation) {
        animation = next;
        animation_frame = animation_ticks = 0;
    } else if (++animation_ticks >= 8 && animation == 1) {
        animation_frame = (animation_frame + 1) & 7;
        animation_ticks = 0;
    }
    render_frame.camera_x = cameraPosition.x;
    render_frame.camera_y = cameraPosition.y;
    render_frame.background_y = (cameraPosition.y + (cameraPosition.y >> 3) + (star_drift >> 1)) & 255;
    render_frame.sprites[0].x = playerBody.x - PLAYER_OFFSET_X - cameraPosition.x;
    render_frame.sprites[0].y = playerBody.y - PLAYER_OFFSET_Y - cameraPosition.y;
    render_frame.sprites[0].flip = playerBody.facingDirection > 0;
    render_frame.sprites[0].frame = animation == 0 ? 0 : animation == 1 ? 1 + animation_frame : animation + 7;
}


const GdlFrame *test_wide_render_frame(void) { return &render_frame; }
