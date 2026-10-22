#include "gdl/level_assets.h"
#include "t_fire_1.h"
#include "player.h"
#include "player_sprite.h"
#include "input_controls.h"
#include "camera.h"
#include "level_collision/collision.h"
#include "level_collision/collision_heightmaps.h"
#include "t_fire1_data.h"

static GDL_LEVEL_ASSETS(fire) uint32_t foreground[sizeof(fire_fg_tiles) / 4];
static GDL_LEVEL_ASSETS(fire) uint16_t foreground_rows[sizeof(fire_fg_rows) / 2];
static GDL_LEVEL_ASSETS(fire) uint16_t foreground_map[sizeof(fire_fg_map) / 2];
static GDL_LEVEL_ASSETS(fire) uint32_t background[sizeof(fire_bg_tiles) / 4];
static GDL_LEVEL_ASSETS(fire) uint16_t background_map[sizeof(fire_bg_map) / 2];
static GDL_LEVEL_ASSETS(fire) uint8_t collision[sizeof(fire_collision)];
static GDL_LEVEL_ASSETS(fire) uint16_t character_rows[sizeof(fire_chakan_rows) / 2];
static GDL_LEVEL_ASSETS(fire) uint16_t character_spans[sizeof(fire_chakan_spans) / 2];
static GDL_LEVEL_ASSETS(fire) uint8_t character_pixels[sizeof(fire_chakan_pixels)];
static const GdlSpanImage actions = {fire_actions_rows, fire_actions_spans, fire_actions_pixels, 112, 80, 5};
static const GdlSpanImage chakan = {character_rows, character_spans, character_pixels, 112, 80, 12};
static const GdlScene scene = {
    {foreground, foreground_map, 200, 232, foreground_rows},
    {background, background_map, 1, 1, 0}
};
static GDL_LEVEL_ASSETS(fire) uint8_t priority[sizeof(fire_fg_priority)];
static GdlFrame frame;
static uint16_t fade_tick;
static void spawn(void) {
    playerInit(FIRE_SPAWN_X + PLAYER_OFFSET_X, FIRE_SPAWN_Y + PLAYER_OFFSET_Y, collision_blocked);
    player_set_collision_move(collision_move_player);
    player_set_collision_queries(collision_can_drop, collision_can_stand);

}
void t_fire1_init(void) {
    gdl_shared_upload(foreground, fire_fg_tiles, sizeof(foreground));
    gdl_shared_upload(foreground_rows, fire_fg_rows, sizeof(foreground_rows));
    gdl_shared_upload(foreground_map, fire_fg_map, sizeof(foreground_map));
    gdl_shared_upload(priority, fire_fg_priority, sizeof(priority));
    gdl_shared_upload(background_map, fire_bg_map, sizeof(background_map));
    gdl_shared_upload(background, fire_bg_tiles, sizeof(background));
    gdl_shared_upload(character_rows, fire_chakan_rows, sizeof(character_rows));
    gdl_shared_upload(character_spans, fire_chakan_spans, sizeof(character_spans));
    gdl_shared_upload(character_pixels, fire_chakan_pixels, sizeof(character_pixels));
    for (uint32_t i = 0; i < sizeof(collision); i++) collision[i] = fire_collision[i];
    collision_bind(collision, 100, 116, tf1_collision_tiles, 8);
    static const uint16_t black[48] = {0};
    gdl_palette_load(0, black, 48);
    gdl_scheduler_init(&scene);
    camera_init(1600, 1856);
    spawn();
    player_sprite_init();
    player_sprite_set_actions(&actions);
    input_init();
    fade_tick = 0;
    frame.foreground_priority = priority;
    frame.background_per_row = false;
    frame.background_per_line = false;
    frame.background_x = frame.background_y = 0;
}
void t_fire1_update(uint16_t pad) {
    processInput(pad);
    updatePlayer();
    if (playerBody.y >= 1856) spawn();
    camera_step(playerBody.x + PLAYER_WIDTH / 2, playerBody.y + playerBody.height / 2);
    if (fade_tick < 20) {
        uint16_t colors[48];
        fade_tick++;
        for (uint16_t i = 0; i < 48; i++) {
            uint16_t c = fire_palette[i];
            colors[i] = ((c & 31) * fade_tick / 20)
                | ((((c >> 5) & 31) * fade_tick / 20) << 5)
                | ((((c >> 10) & 31) * fade_tick / 20) << 10);
        }
        gdl_palette_schedule(0, colors, 48);
    }
    frame.camera_x = cameraPosition.x;
    frame.camera_y = cameraPosition.y;
    frame.sprite_count = 0;
    player_sprite_submit(&frame, &chakan, cameraPosition.x, cameraPosition.y);
    frame.sprites[0].behind_foreground = GDL_SPRITE_BEHIND_PRIORITY;
}
const GdlFrame *t_fire1_render_frame(void) { return &frame; }
