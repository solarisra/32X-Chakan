#ifndef CHAKAN_SH_PORTAL_HUB_H
#define CHAKAN_SH_PORTAL_HUB_H
#include "gdl/gdl.h"
#include "level_manager.h"
void portal_hub_init(void);
void portal_hub_update(uint16_t pad);
const GdlFrame *portal_hub_render_frame(void);
/* Fresh Up while overlapping an entrance queues one destination. */
GameLevel portal_hub_take_destination(void);
/* Raw collision-map IDs: 2..6 mark portals with source behavior IDs 1..5.
 * Behavior descriptors are reported by collision_player_behavior(). */
uint8_t portal_hub_collision_at(int16_t tile_x, int16_t tile_y);
#endif
