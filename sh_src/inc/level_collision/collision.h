#ifndef CHAKAN_SH_COLLISION_H
#define CHAKAN_SH_COLLISION_H
#include "player.h"
#include "level_collision/collision_tile_types.h"
#define ONE_WAY_PLATFORM_ERROR_CORRECTION 1
#define MAX_STEP_UP 4
#define MAX_STEP_DOWN 8
#define CEILING_NUDGE_MAX 6
#define NO_FLOOR_SURFACE_Y 32767
typedef struct { s16 x, y; } CollisionPoint;
typedef struct { CollisionPoint min, max; } AABB;
typedef struct {
    const uint8_t *cells;
    uint16_t map_tile_width, map_tile_height, collision_tile_count;
    const TileCollisionData *const *collision_tiles;
} Level;
typedef enum { COLLISION_NONE=0, COLLISION_FLOOR=1, COLLISION_CEILING=2,
               COLLISION_LEFT=4, COLLISION_RIGHT=8 } CollisionFlags;
typedef struct {
    s16 vx, vy;
    u16 flags, specialType;
    s16 hit_tx, hit_ty;
    bool ignore_one_way;
} CollisionResult;
extern CollisionResult player_collision_result;
void collision_bind(const uint8_t *cells, uint16_t width, uint16_t height,
                    const TileCollisionData *const *tiles, uint16_t count);
const TileCollisionData *get_tile_collision_data(const Level *, s16, s16);
bool collision_aabb_overlaps_solid(const Level *, const AABB *, bool ignore_one_way);
bool collision_aabb_floor_is_only_one_way_or_empty(const Level *, const AABB *);
const TileBehaviorData *find_overlapping_tile_behavior(const Level *, const AABB *, TileBehaviorType);
void resolve_level_collisions(Level *, AABB *, CollisionResult *);
bool collision_blocked(int16_t x, int16_t y);
bool collision_can_stand(int16_t x, int16_t y);
bool collision_can_drop(void);
void collision_move_player(int16_t dx, int16_t dy);
const TileBehaviorData *collision_player_behavior(TileBehaviorType type);
#endif
