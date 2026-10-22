#include "gdl/level_assets.h"
#include "t_air_1.h"
#include "player.h"
#include "player_sprite.h"
#include "input_controls.h"
#include "camera.h"
#include "level_collision/collision.h"
#include "level_collision/collision_heightmaps.h"
#include "t_air1_data.h"

static GDL_LEVEL_ASSETS(air) uint32_t foreground[sizeof(air_fg_tiles) / 4];
static GDL_LEVEL_ASSETS(air) uint16_t foreground_rows[sizeof(air_fg_rows) / 2];
static GDL_LEVEL_ASSETS(air) uint16_t foreground_map[sizeof(air_fg_map) / 2];
static GDL_LEVEL_ASSETS(air) uint32_t background[sizeof(air_bg_tiles) / 4];
static GDL_LEVEL_ASSETS(air) uint16_t background_map[sizeof(air_bg_map) / 2];
static GDL_LEVEL_ASSETS(air) uint8_t collision[sizeof(air_collision)];
static GDL_LEVEL_ASSETS(air) uint16_t character_rows[sizeof(air_chakan_rows) / 2];
static GDL_LEVEL_ASSETS(air) uint16_t character_spans[sizeof(air_chakan_spans) / 2];
static GDL_LEVEL_ASSETS(air) uint8_t character_pixels[sizeof(air_chakan_pixels)];
static const GdlSpanImage actions = {air_actions_rows, air_actions_spans, air_actions_pixels, 112, 80, 5};
static const GdlSpanImage chakan = {character_rows, character_spans, character_pixels, 112, 80, 12};
static const GdlScene scene = {
    {foreground, foreground_map, 240, 180, foreground_rows},
    {background, background_map, 32, 28, 0}
};
static GDL_LEVEL_ASSETS(air) uint8_t priority[sizeof(air_fg_priority)];
static GdlFrame frame;
static uint16_t fade_tick;
/* One 8.8 phase per visible 8px row; zero-speed center band stays still. */
static uint16_t row_phase[GDL_HEIGHT / 8];
static uint16_t row_speed(uint16_t row) {
    if (row <= 10) return 64 + (10-row)*64;
    if (row >= 17) return 64 + (row-17)*64;
    return 0;
}

static void spawn(void) {
    playerInit(AIR_SPAWN_X + PLAYER_OFFSET_X, AIR_SPAWN_Y + PLAYER_OFFSET_Y, collision_blocked);
    player_set_collision_move(collision_move_player);
    player_set_collision_queries(collision_can_drop, collision_can_stand);

}
void t_air1_init(void) {
    gdl_shared_upload(foreground, air_fg_tiles, sizeof(foreground));
    gdl_shared_upload(foreground_rows, air_fg_rows, sizeof(foreground_rows));
    gdl_shared_upload(foreground_map, air_fg_map, sizeof(foreground_map));
    gdl_shared_upload(priority, air_fg_priority, sizeof(priority));
    gdl_shared_upload(background_map, air_bg_map, sizeof(background_map));
    gdl_shared_upload(background, air_bg_tiles, sizeof(background));
    gdl_shared_upload(character_rows, air_chakan_rows, sizeof(character_rows));
    gdl_shared_upload(character_spans, air_chakan_spans, sizeof(character_spans));
    gdl_shared_upload(character_pixels, air_chakan_pixels, sizeof(character_pixels));
    for (uint32_t i = 0; i < sizeof(collision); i++) collision[i] = air_collision[i];
    collision_bind(collision, 120, 90, ta1_collision_tiles, 5);
    static const uint16_t black[64] = {0};
    gdl_palette_load(0, black, 64);
    gdl_scheduler_init(&scene);
    camera_init(1920, 1440);
    spawn();
    player_sprite_init();
    player_sprite_set_actions(&actions);
    input_init();
    fade_tick = 0;
    frame.foreground_priority = priority;
    frame.background_per_row = true;
    frame.background_y = 0;
    for (uint16_t row=0; row<GDL_HEIGHT/8; row++) row_phase[row]=0;
}
void t_air1_update(uint16_t pad) {
    processInput(pad);
    updatePlayer();
    if (playerBody.y >= 1440) spawn();
    camera_step(playerBody.x + PLAYER_WIDTH / 2, playerBody.y + playerBody.height / 2);
    if (fade_tick < 20) {
        uint16_t colors[64];
        fade_tick++;
        for (uint16_t i = 0; i < 64; i++) {
            uint16_t c = air_palette[i];
            colors[i] = ((c & 31) * fade_tick / 20)
                | ((((c >> 5) & 31) * fade_tick / 20) << 5)
                | ((((c >> 10) & 31) * fade_tick / 20) << 10);
        }
        gdl_palette_schedule(0, colors, 64);
    }
    frame.camera_x = cameraPosition.x;
    frame.camera_y = cameraPosition.y;
    frame.background_x = cameraPosition.x / 8;
    for (uint16_t row=0; row<GDL_HEIGHT/8; row++) {
        row_phase[row] += row_speed(row);
        frame.background_rows[row] = (frame.background_x - (row_phase[row] >> 8)) & 255;
    }
    frame.sprite_count = 0;
    player_sprite_submit(&frame, &chakan, cameraPosition.x, cameraPosition.y);
    frame.sprites[0].behind_foreground = GDL_SPRITE_BEHIND_PRIORITY;
}
const GdlFrame *t_air1_render_frame(void) { return &frame; }
