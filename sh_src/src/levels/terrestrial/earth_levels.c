#include "gdl/level_assets.h"
#include "earth_levels.h"
#include "player.h"
#include "player_sprite.h"
#include "input_controls.h"
#include "camera.h"
#include "level_collision/collision.h"
#include "level_collision/collision_heightmaps.h"
#include "t_earth1_data.h"

static GDL_LEVEL_ASSETS(earth) uint32_t foreground[sizeof(earth_fg_tiles) / 4];
static GDL_LEVEL_ASSETS(earth) uint16_t foreground_rows[sizeof(earth_fg_rows) / 2];
static GDL_LEVEL_ASSETS(earth) uint16_t foreground_map[sizeof(earth_fg_map) / 2];
static GDL_LEVEL_ASSETS(earth) uint32_t background[sizeof(earth_bg_tiles) / 4];
static GDL_LEVEL_ASSETS(earth) uint16_t background_map[sizeof(earth_bg_map) / 2];
static GDL_LEVEL_ASSETS(earth) uint8_t collision[sizeof(earth_collision)];
static GDL_LEVEL_ASSETS(earth) uint16_t character_rows[sizeof(earth_chakan_rows) / 2];
static GDL_LEVEL_ASSETS(earth) uint16_t character_spans[sizeof(earth_chakan_spans) / 2];
static GDL_LEVEL_ASSETS(earth) uint8_t character_pixels[sizeof(earth_chakan_pixels)];
static const GdlSpanImage actions = {earth_actions_rows, earth_actions_spans, earth_actions_pixels, 112, 80, 5};
static const GdlSpanImage chakan = {character_rows, character_spans, character_pixels, 112, 80, 12};
static const GdlScene scene = {
    {foreground, foreground_map, 322, 108, foreground_rows},
    {background, background_map, 32, 28, 0}
};
static GdlFrame frame;
static uint16_t fade_tick;
/* Independent background state: X uses half-pixel units for smooth 1:2 motion.
 * Camera deltas drive this plane; offsets remain independent renderer inputs. */
static uint16_t background_half_x, background_scroll_y;
static int16_t background_camera_x, background_camera_y;

static void spawn(void) {
    playerInit(EARTH_SPAWN_X + PLAYER_OFFSET_X, EARTH_SPAWN_Y + PLAYER_OFFSET_Y, collision_blocked);
    player_set_collision_move(collision_move_player);
    player_set_collision_queries(collision_can_drop, collision_can_stand);
    background_half_x = background_scroll_y = 0;
    background_camera_x = cameraPosition.x;
    background_camera_y = cameraPosition.y;
}
void t_earth1_init(void) {
    gdl_shared_upload(foreground, earth_fg_tiles, sizeof(foreground));
    gdl_shared_upload(foreground_rows, earth_fg_rows, sizeof(foreground_rows));
    gdl_shared_upload(foreground_map, earth_fg_map, sizeof(foreground_map));
    gdl_shared_upload(background_map, earth_bg_map, sizeof(background_map));
    gdl_shared_upload(background, earth_bg_tiles, sizeof(background));
    gdl_shared_upload(character_rows, earth_chakan_rows, sizeof(character_rows));
    gdl_shared_upload(character_spans, earth_chakan_spans, sizeof(character_spans));
    gdl_shared_upload(character_pixels, earth_chakan_pixels, sizeof(character_pixels));
    for (uint32_t i = 0; i < sizeof(collision); i++) collision[i] = earth_collision[i];
    collision_bind(collision, 161, 54, te1_collision_tiles, 10);
    static const uint16_t black[48] = {0};
    gdl_palette_load(0, black, 48);
    gdl_scheduler_init(&scene);
    camera_init(2576, 864);
    spawn();
    player_sprite_init();
    player_sprite_set_actions(&actions);
    input_init();
    fade_tick = 0;
}
void t_earth1_update(uint16_t pad) {
    processInput(pad);
    updatePlayer();
    if (playerBody.y >= 864) spawn();
    camera_step(playerBody.x + PLAYER_WIDTH / 2, playerBody.y + playerBody.height / 2);
    if (fade_tick < 20) {
        uint16_t colors[48];
        fade_tick++;
        for (uint16_t i = 0; i < 48; i++) {
            uint16_t c = earth_palette[i];
            colors[i] = ((c & 31) * fade_tick / 20)
                | ((((c >> 5) & 31) * fade_tick / 20) << 5)
                | ((((c >> 10) & 31) * fade_tick / 20) << 10);
        }
        gdl_palette_schedule(0, colors, 48);
    }
    frame.camera_x = cameraPosition.x;
    frame.camera_y = cameraPosition.y;
    int16_t dx = cameraPosition.x - background_camera_x;
    int16_t dy = cameraPosition.y - background_camera_y;
    background_half_x = (uint16_t)(background_half_x + dx) & 511;
    int16_t next_y = (background_scroll_y + dy) % 224;
    background_scroll_y = next_y < 0 ? next_y + 224 : next_y;
    background_camera_x = cameraPosition.x;
    background_camera_y = cameraPosition.y;
    frame.background_x = background_half_x / 2;
    frame.background_y = background_scroll_y;
    frame.sprite_count = 0;
    player_sprite_submit(&frame, &chakan, cameraPosition.x, cameraPosition.y);
}
const GdlFrame *t_earth1_render_frame(void) { return &frame; }
