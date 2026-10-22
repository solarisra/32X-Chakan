#ifndef CHAKAN_SH_COLLISION_HEIGHTMAPS_H
#define CHAKAN_SH_COLLISION_HEIGHTMAPS_H
#include "collision_tile_types.h"
/* Earth 1 tile table: empty, full solid, floor-only 16h/8h, four 2w slopes,
 * and explicitly one-way 16h/8h platforms (IDs 8/9).
 * Heights count upward from the bottom of a 16-pixel collision cell. */
extern const uint8_t te1_collision_heightmaps[10][16];
extern const TileCollisionData *const te1_collision_tiles[10];
extern const TileCollisionData *const portal_hub_collision_tiles[7];
extern const TileCollisionData *const ta1_collision_tiles[5];
extern const TileCollisionData *const tw1_collision_tiles[27];
extern const TileCollisionData *const tf1_collision_tiles[8];
#endif
