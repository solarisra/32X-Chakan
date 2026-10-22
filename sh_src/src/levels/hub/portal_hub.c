#include "gdl/level_assets.h"
#include "portal_hub.h"
#include "hub_sprites.h"
#include "level_progress.h"
#include "player.h"
#include "level_collision/collision.h"
#include "level_collision/collision_heightmaps.h"
#include "input_controls.h"
#include "camera.h"
#include "portal_hub_data.h"

static GDL_LEVEL_ASSETS(hub) uint32_t foreground[sizeof(hub_fg_tiles) / 4], background[sizeof(hub_bg_tiles) / 4];
static GDL_LEVEL_ASSETS(hub) uint16_t background_map[sizeof(hub_bg_map) / 2];
static GDL_LEVEL_ASSETS(hub) uint16_t foreground_map[sizeof(hub_fg_map) / 2], foreground_rows[sizeof(hub_fg_rows) / 2];
static GDL_LEVEL_ASSETS(hub) uint8_t collision[sizeof(hub_collision)];
static GdlFrame frame;
static uint32_t ticks;
static uint16_t cycling_colors[5];
static GameLevel destination;
static const CameraRegion central_region={704,896,500,800};
static bool camera_transition_active;
GameLevel portal_hub_take_destination(void) {
    GameLevel result=destination;
    destination=LEVEL_PORTAL_HUB;
    return result;
}
static GameLevel interaction_destination(void) {
    const TileBehaviorData *door=collision_player_behavior(TILE_BEHAVIOR_DOOR);
    if (door) {
        if (level_progress_realm(1)) return LEVEL_FINAL;
        switch (door->id) {
        case 1: return level_progress_next(3);
        case 2: return level_progress_next(0);
        case 3: return level_progress_next(1);
        case 4: return level_progress_next(2);
        default: return LEVEL_PORTAL_HUB; /* Elemental entrance is not migrated. */
        }
    }
    int16_t portal=hub_portal_overlap(playerBody.x,playerBody.y,PLAYER_WIDTH,playerBody.height);
    return portal<0?LEVEL_PORTAL_HUB:level_progress_next(portal);
}
static const GdlScene scene = {{foreground, foreground_map, 240, 160, foreground_rows}, {background, background_map, 64, 32, 0}};
uint8_t portal_hub_collision_at(int16_t x, int16_t y) {
    if (x < 0 || x >= 120 || y < 0 || y >= 80) return 0;
    return collision[y * 120 + x];
}
#define blocked collision_blocked
void portal_hub_init(void) {
    gdl_shared_upload(foreground, hub_fg_tiles, sizeof(foreground));
    gdl_shared_upload(foreground_map, hub_fg_map, sizeof(foreground_map));
    gdl_shared_upload(foreground_rows, hub_fg_rows, sizeof(foreground_rows));
    gdl_shared_upload(background, hub_bg_tiles, sizeof(background));
    gdl_shared_upload(background_map, hub_bg_map, sizeof(background_map));
    for (uint32_t i = 0; i < sizeof(collision); i++) collision[i] = hub_collision[i];
    static const uint16_t black_palette[80] = {0};
    gdl_palette_load(0, black_palette, 80);
    for (uint16_t i = 0; i < 5; i++) cycling_colors[i] = hub_palette[27 + i];
    hub_sprites_init();
    gdl_scheduler_init(&scene);
    collision_bind(collision, 120, 80, portal_hub_collision_tiles, 7);
    playerInit(HUB_SPAWN_X + PLAYER_OFFSET_X, HUB_SPAWN_Y + PLAYER_OFFSET_Y, blocked);
    player_set_collision_move(collision_move_player);
    player_set_collision_queries(collision_can_drop, collision_can_stand);
    input_init();
    camera_init(1920, 1280);
    camera_step(playerBody.x+PLAYER_WIDTH/2,playerBody.y+playerBody.height/2);
    camera_transition_active=camera_in_region(playerBody.x,playerBody.y,&central_region);
    if (camera_transition_active) camera_set_y(580);
    ticks = 0;
    destination=LEVEL_PORTAL_HUB;
}
void portal_hub_update(uint16_t pad) {
    bool interact=(pad&BUTTON_UP) && !(prevJoyInput&BUTTON_UP)
        && player_is_control_enabled() && player_is_simulation_enabled();
    processInput(pad);
    updatePlayer();
    if (interact) destination=interaction_destination();
    if (playerBody.y >= 1280) {
        playerInit(HUB_SPAWN_X + PLAYER_OFFSET_X, HUB_SPAWN_Y + PLAYER_OFFSET_Y, blocked);
        player_set_collision_move(collision_move_player);
        player_set_collision_queries(collision_can_drop, collision_can_stand);
    }
    int16_t previous_camera_y=cameraPosition.y;
    camera_step(playerBody.x + PLAYER_WIDTH / 2, playerBody.y + playerBody.height / 2);
    camera_apply_y_region_lock(previous_camera_y,playerBody.x,playerBody.y,
                              &central_region,580,4,&camera_transition_active);
    ticks++;
    if (ticks <= 20) gdl_palette_schedule(0, hub_fade_palette + (ticks - 1) * 80, 80);
    if (ticks > 20 && (ticks & 7) == 0) {
        uint16_t first = cycling_colors[0];
        for (uint16_t i = 0; i < 4; i++) cycling_colors[i] = cycling_colors[i + 1];
        cycling_colors[4] = first;
        gdl_palette_schedule(27, cycling_colors, 5);
    }
    frame.camera_x = cameraPosition.x;
    frame.background_x = cameraPosition.x;
    frame.camera_y = cameraPosition.y;
    frame.background_y = (cameraPosition.y + (cameraPosition.y >> 3) + ((ticks >> 1) & 255)) & 255;
    hub_sprites_submit(&frame, ticks);
}
const GdlFrame *portal_hub_render_frame(void) { return &frame; }
