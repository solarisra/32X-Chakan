#include "portal_hub.h"
#include "hub_sprites.h"
#include "player.h"
#include "input_controls.h"
#include "camera.h"
#include "portal_hub_data.h"

static uint32_t foreground[sizeof(hub_fg_tiles) / 4], background[sizeof(hub_bg_tiles) / 4];
static uint16_t background_map[sizeof(hub_bg_map) / 2];
static uint16_t foreground_map[sizeof(hub_fg_map) / 2], foreground_rows[sizeof(hub_fg_rows) / 2];
static uint8_t collision[sizeof(hub_collision)];
static GdlFrame frame;
static uint32_t ticks;
static uint16_t cycling_colors[5];
static const GdlScene scene = {{foreground, foreground_map, 240, 160, foreground_rows}, {background, background_map, 64, 32, 0}};
uint8_t portal_hub_collision_at(int16_t x, int16_t y) {
    if (x < 0 || x >= 120 || y < 0 || y >= 80) return 0;
    return collision[y * 120 + x];
}
static bool blocked(int16_t x, int16_t y) {
    if (x < 0 || x + PLAYER_WIDTH > 1920 || y < 0) return true;
    for (int16_t ty = y / 16; ty <= (y + PLAYER_HEIGHT - 1) / 16 && ty < 80; ty++)
        for (int16_t tx = x / 16; tx <= (x + PLAYER_WIDTH - 1) / 16; tx++)
            if (portal_hub_collision_at(tx, ty) == 1) return true;
    return false;
}
void portal_hub_init(void) {
    gdl_shared_upload(foreground, hub_fg_tiles, sizeof(foreground));
    gdl_shared_upload(foreground_map, hub_fg_map, sizeof(foreground_map));
    gdl_shared_upload(foreground_rows, hub_fg_rows, sizeof(foreground_rows));
    gdl_shared_upload(background, hub_bg_tiles, sizeof(background));
    gdl_shared_upload(background_map, hub_bg_map, sizeof(background_map));
    for (uint32_t i = 0; i < sizeof(collision); i++) collision[i] = hub_collision[i];
    static const uint16_t black_palette[64] = {0};
    gdl_palette_load(0, black_palette, 64);
    for (uint16_t i = 0; i < 5; i++) cycling_colors[i] = hub_palette[27 + i];
    hub_sprites_init();
    gdl_scheduler_init(&scene);
    playerInit(HUB_SPAWN_X + PLAYER_OFFSET_X, HUB_SPAWN_Y + PLAYER_OFFSET_Y, blocked);
    input_init();
    camera_init(1920, 1280);
    ticks = 0;
}
void portal_hub_update(uint16_t pad) {
    processInput(pad);
    updatePlayer();
    if (playerBody.y >= 1280)
        playerInit(HUB_SPAWN_X + PLAYER_OFFSET_X, HUB_SPAWN_Y + PLAYER_OFFSET_Y, blocked);
    camera_step(playerBody.x + PLAYER_WIDTH / 2, playerBody.y + PLAYER_HEIGHT / 2);
    ticks++;
    if (ticks <= 20) gdl_palette_schedule(0, hub_fade_palette + (ticks - 1) * 64, 64);
    if (ticks > 20 && (ticks & 7) == 0) {
        uint16_t first = cycling_colors[0];
        for (uint16_t i = 0; i < 4; i++) cycling_colors[i] = cycling_colors[i + 1];
        cycling_colors[4] = first;
        gdl_palette_schedule(27, cycling_colors, 5);
    }
    frame.camera_x = cameraPosition.x;
    frame.camera_y = cameraPosition.y;
    frame.background_y = (cameraPosition.y + (cameraPosition.y >> 3) + ((ticks >> 1) & 255)) & 255;
    hub_sprites_submit(&frame, ticks);
}
const GdlFrame *portal_hub_render_frame(void) { return &frame; }
