#include "gdl/level_assets.h"
#include "t_water_1.h"
#include "player.h"
#include "player_sprite.h"
#include "input_controls.h"
#include "camera.h"
#include "level_collision/collision.h"
#include "level_collision/collision_heightmaps.h"
#include "t_water1_data.h"

static GDL_LEVEL_ASSETS(water) uint32_t foreground[sizeof(water_fg_tiles) / 4];
static GDL_LEVEL_ASSETS(water) uint16_t foreground_rows[sizeof(water_fg_rows) / 2];
static GDL_LEVEL_ASSETS(water) uint16_t foreground_map[sizeof(water_fg_map) / 2];
/* Keep raster data in SDRAM; immutable Chakan spans stay in ROM to fit. */
static GDL_LEVEL_ASSETS(water) uint32_t background[sizeof(water_bg_tiles)/4];
static GDL_LEVEL_ASSETS(water) uint16_t background_rows[sizeof(water_bg_rows) / 2];
static GDL_LEVEL_ASSETS(water) uint8_t foreground_priority[sizeof(water_fg_priority)], background_priority[sizeof(water_bg_priority)];
static GDL_LEVEL_ASSETS(water) uint8_t opaque_rows[sizeof(water_bg_opaque_rows)];
static GDL_LEVEL_ASSETS(water) uint16_t background_map[sizeof(water_bg_map) / 2];
static GDL_LEVEL_ASSETS(water) uint8_t collision[sizeof(water_collision)];
static GDL_LEVEL_ASSETS(water) uint16_t character_rows[sizeof(water_chakan_rows) / 2];
static GDL_LEVEL_ASSETS(water) uint8_t character_pixels[sizeof(water_chakan_pixels)];
static const GdlSpanImage actions = {water_actions_rows, water_actions_spans, water_actions_pixels, 112, 80, 5};
static const GdlSpanImage chakan = {character_rows, water_chakan_spans, character_pixels, 112, 80, 12};
static const GdlScene scene = {
    {foreground, foreground_map, 800, 62, foreground_rows},
    {background, background_map, 64, 62, background_rows}
};
static GdlFrame frame;
static uint16_t fade_tick;
/* Water phases use 8.8 precision; animated drift is tuned to 81% of the source.
 * Camera parallax is independent and phases wrap across the 512px image. */
static uint16_t gradient_speed[95];
#define WATER_DRIFT_PERCENT 81
static uint32_t gradient_phase[95], fast_phase;
static void reset_raster(void) {
    uint16_t speed=128,error=0;
    for (uint16_t i=0;i<95;i++) {
        gradient_speed[i]=(speed*WATER_DRIFT_PERCENT+50)/100; gradient_phase[i]=0;
        error+=384;
        while (error>=94) { speed++; error-=94; }
    }
    fast_phase=0;
}

static void spawn(void) {
    playerInit(WATER_SPAWN_X + PLAYER_OFFSET_X, WATER_SPAWN_Y + PLAYER_OFFSET_Y, collision_blocked);
    player_set_collision_move(collision_move_player);
    player_set_collision_queries(collision_can_drop, collision_can_stand);

}
void t_water1_init(void) {
    gdl_shared_upload(foreground, water_fg_tiles, sizeof(foreground));
    gdl_shared_upload(foreground_rows, water_fg_rows, sizeof(foreground_rows));
    gdl_shared_upload(foreground_map, water_fg_map, sizeof(foreground_map));
    gdl_shared_upload(background_map, water_bg_map, sizeof(background_map));
    gdl_shared_upload(background,water_bg_tiles,sizeof(background));
    gdl_shared_upload(background_rows, water_bg_rows, sizeof(background_rows));
    gdl_shared_upload(foreground_priority,water_fg_priority,sizeof(foreground_priority));
    gdl_shared_upload(opaque_rows,water_bg_opaque_rows,sizeof(opaque_rows));
    gdl_shared_upload(background_priority,water_bg_priority,sizeof(background_priority));
    gdl_shared_upload(character_rows, water_chakan_rows, sizeof(character_rows));
    gdl_shared_upload(character_pixels, water_chakan_pixels, sizeof(character_pixels));
    for (uint32_t i = 0; i < sizeof(collision); i++) collision[i] = water_collision[i];
    collision_bind(collision, 400, 31, tw1_collision_tiles, 27);
    static const uint16_t black[48] = {0};
    gdl_palette_load(0, black, 48);
    gdl_scheduler_init(&scene);
    camera_init(6400, 496);
    spawn();
    player_sprite_init();
    player_sprite_set_actions(&actions);
    input_init();
    fade_tick = 0;
    reset_raster();
    frame.background_per_line=true;
    frame.background_pixel_priority=true;
    frame.foreground_map_priority=true;
    frame.foreground_priority=foreground_priority;
    frame.background_priority=background_priority;
    frame.background_opaque_rows=opaque_rows;
}
void t_water1_update(uint16_t pad) {
    processInput(pad);
    updatePlayer();
    if (playerBody.y >= 496) spawn();
    camera_step(playerBody.x + PLAYER_WIDTH / 2, playerBody.y + playerBody.height / 2);
    if (fade_tick < 20) {
        uint16_t colors[48];
        fade_tick++;
        for (uint16_t i = 0; i < 48; i++) {
            uint16_t c = water_palette[i];
            colors[i] = ((c & 31) * fade_tick / 20)
                | ((((c >> 5) & 31) * fade_tick / 20) << 5)
                | ((((c >> 10) & 31) * fade_tick / 20) << 10);
        }
        gdl_palette_schedule(0, colors, 48);
    }
    frame.camera_x = cameraPosition.x;
    frame.camera_y = cameraPosition.y;
    for (uint16_t i=0;i<95;i++) gradient_phase[i]=(gradient_phase[i]+gradient_speed[i])&131071UL;
    fast_phase=(fast_phase+(640*WATER_DRIFT_PERCENT+50)/100)&131071UL; /* About 2.02px/tick. */
    frame.background_x=cameraPosition.x/2;
    frame.background_y=cameraPosition.y;
    for (uint16_t y=0;y<GDL_HEIGHT;y++) {
        uint16_t world_y=cameraPosition.y+y;
        uint16_t drift=world_y>=384?fast_phase>>8:
            world_y>=289?gradient_phase[world_y-289]>>8:0;
        frame.background_rows[y]=(frame.background_x+drift)&511;
    }
    frame.sprite_count = 0;
    player_sprite_submit(&frame, &chakan, cameraPosition.x, cameraPosition.y);
    frame.sprites[0].behind_foreground=GDL_SPRITE_BEHIND_PRIORITY;
}
const GdlFrame *t_water1_render_frame(void) { return &frame; }
