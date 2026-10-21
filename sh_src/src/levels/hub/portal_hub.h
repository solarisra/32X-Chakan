#ifndef CHAKAN_SH_PORTAL_HUB_H
#define CHAKAN_SH_PORTAL_HUB_H
#include "gdl/gdl.h"
void portal_hub_init(void);
void portal_hub_update(uint16_t pad);
const GdlFrame *portal_hub_render_frame(void);
/* Raw collision-map IDs: 2..6 mark portals with source behavior IDs 1..5.
 * Behavior descriptors and overlap reporting are not yet migrated. */
uint8_t portal_hub_collision_at(int16_t tile_x, int16_t tile_y);
#endif
