/* Collision resolution ported from Chakan_Remake_v1 create_level_stages.
 * SGDK's window allocator is replaced by a bounds-checked resident grid.
 * AABBs in the resolver have inclusive max edges. */
#include "level_collision/collision.h"
#define min(a,b) ((a)<(b)?(a):(b))
#define max(a,b) ((a)>(b)?(a):(b))
static Level active_level;
CollisionResult player_collision_result;
static void decompress_collision_window(const Level *level, u16 x, u16 y) {
    (void)level; (void)x; (void)y;
}
const TileCollisionData *get_tile_collision_data(const Level *level, s16 x, s16 y) {
    if (x < 0 || y < 0 || x >= level->map_tile_width || y >= level->map_tile_height) return NULL;
    u8 id = level->cells[y * level->map_tile_width + x];
    return id < level->collision_tile_count ? level->collision_tiles[id] : NULL;
}
static inline bool tile_is_solid_at_world_point(const TileCollisionData* tile_collision_data,
                                                s16 tile_x,
                                                s16 tile_y,
                                                s16 world_x,
                                                s16 world_y)
{
    if (!tile_collision_data || !tile_collision_data->geometry) return FALSE;
    const TileGeometryData* tile_geometry = tile_collision_data->geometry;
    if (tile_geometry->shape == TILE_SHAPE_EMPTY) return FALSE;
    const s16 tile_world_x0 = (s16)(tile_x << 4);
    const s16 tile_world_y0 = (s16)(tile_y << 4);
    const s16 local_x = (s16)(((u16)(world_x - tile_world_x0)) & 15);
    const s16 local_y = (s16)(((u16)(world_y - tile_world_y0)) & 15);
    if (tile_geometry->shape == TILE_SHAPE_FULL)
        return TRUE;
    if (tile_geometry->shape == TILE_SHAPE_HMAP && tile_geometry->hmap)
    {
        u8 column_height = tile_geometry->hmap[(u8)local_x];
        if (column_height == 0) return FALSE;
        if (column_height > 16) column_height = 16;
        const s16 solid_top_local_y = (s16)(16 - (s16)column_height);
        return (local_y >= solid_top_local_y) ? TRUE : FALSE;
    }
    return FALSE;
}
bool collision_aabb_overlaps_solid(const Level* level, const AABB* aabb, bool ignore_one_way)
{
    if (!level || !aabb)
        return FALSE;
    const s16 center_tx = (s16)(((aabb->min.x + aabb->max.x) >> 1) >> 4);
    const s16 center_ty = (s16)(((aabb->min.y + aabb->max.y) >> 1) >> 4);
    decompress_collision_window(level, (u16)center_tx, (u16)center_ty);
    const s16 min_tx = (s16)((aabb->min.x + 1) >> 4);
    const s16 max_tx = (s16)((aabb->max.x - 1) >> 4);
    const s16 min_ty = (s16)((aabb->min.y + 1) >> 4);
    const s16 max_ty = (s16)((aabb->max.y - 1) >> 4);
    for (s16 ty = min_ty; ty <= max_ty; ty++)
    {
        for (s16 tx = min_tx; tx <= max_tx; tx++)
        {
            const TileCollisionData* tile = get_tile_collision_data(level, tx, ty);
            if (!tile || !tile->geometry)
                continue;
            const TileGeometryData* tile_geometry = tile->geometry;
            if (tile_geometry->shape == TILE_SHAPE_EMPTY)
                continue;
            if (ignore_one_way && (tile_geometry->flags & TILE_FLAG_ONE_WAY))
                continue;
            if ((tile_geometry->flags & FLAGS_SOLID) == 0)
                continue;
            if (tile_geometry->shape == TILE_SHAPE_FULL)
                return TRUE;
            if (tile_geometry->shape == TILE_SHAPE_HMAP && tile_geometry->hmap)
            {
                const s16 tile_left = (s16)(tx << 4);
                const s16 tile_top = (s16)(ty << 4);
                const s16 start_x = max(aabb->min.x, tile_left);
                const s16 end_x = min(aabb->max.x, (s16)(tile_left + 15));
                for (s16 world_x = start_x; world_x <= end_x; world_x++)
                {
                    const s16 local_x = (s16)(world_x - tile_left);
                    u8 column_height = tile_geometry->hmap[(u8)local_x];
                    if (column_height == 0)
                        continue;
                    if (column_height > 16)
                        column_height = 16;
                    const s16 solid_top = (s16)(tile_top + (16 - (s16)column_height));
                    const s16 solid_bottom = (s16)(tile_top + 15);
                    if (aabb->max.y >= solid_top && aabb->min.y <= solid_bottom)
                        return TRUE;
                }
            }
        }
    }
    return FALSE;
}
bool collision_aabb_floor_is_only_one_way_or_empty(const Level* level, const AABB* aabb)
{
    if (!level || !aabb)
        return FALSE;
    const s16 foot_y = (s16)(aabb->max.y + 1);
    const s16 min_tx = (s16)((aabb->min.x + 1) >> 4);
    const s16 max_tx = (s16)((aabb->max.x - 1) >> 4);
    const s16 ty = (s16)(foot_y >> 4);
    const s16 center_tx = (s16)(((aabb->min.x + aabb->max.x) >> 1) >> 4);
    decompress_collision_window(level, (u16)center_tx, (u16)ty);
    for (s16 tx = min_tx; tx <= max_tx; tx++)
    {
        const TileCollisionData* tile = get_tile_collision_data(level, tx, ty);
        if (!tile || !tile->geometry)
            continue;
        if (tile->geometry->shape == TILE_SHAPE_EMPTY)
            continue;
        if ((tile->geometry->flags & TILE_FLAG_ONE_WAY) == 0)
            return FALSE;
    }
    return TRUE;
}
static inline s16 tile_floor_surface_y_at_x(const TileCollisionData* tile_collision_data,
                                            s16 tile_x,
                                            s16 tile_y,
                                            s16 world_x)
{
    if (!tile_collision_data || !tile_collision_data->geometry) return NO_FLOOR_SURFACE_Y;
    const TileGeometryData* tile_geometry = tile_collision_data->geometry;
    if (!(tile_geometry->flags & TILE_FLAG_BLOCK_DOWN)) return NO_FLOOR_SURFACE_Y;
    const s16 tile_top_world_y = (s16)(tile_y << 4);
    if (tile_geometry->shape == TILE_SHAPE_FULL)
        return tile_top_world_y;
    if (tile_geometry->shape == TILE_SHAPE_HMAP && tile_geometry->hmap)
    {
        const s16 tile_left_world_x = (s16)(tile_x << 4);
        const s16 local_x = (s16)(world_x - tile_left_world_x);
        if (local_x < 0 || local_x > 15) return NO_FLOOR_SURFACE_Y;
        u8 column_height = tile_geometry->hmap[(u8)local_x];
        if (column_height == 0) return NO_FLOOR_SURFACE_Y;
        if (column_height > 16) column_height = 16;
        return (s16)(tile_top_world_y + (16 - (s16)column_height));
    }
    return NO_FLOOR_SURFACE_Y;
}
typedef struct
{
    u16 geom_flags_hit;
    u8  saw_hmap;
    const TileBehaviorData* behavior;
    s16 beh_tx, beh_ty;
} CollisionTileScanResult;
static void scan_overlapping_tiles_for_horizontal(const Level* level, const AABB* aabb, s16 world_vx, CollisionTileScanResult* out_scan)
{
    out_scan->geom_flags_hit = 0;
    out_scan->saw_hmap = 0;
    out_scan->behavior = NULL;
    out_scan->beh_tx = 0;
    out_scan->beh_ty = 0;
    (void)world_vx;
    const s16 min_tile_x = (s16)(aabb->min.x >> 4);
    const s16 max_tile_x = (s16)(aabb->max.x >> 4);
    const s16 min_tile_y = (s16)(aabb->min.y >> 4);
    const s16 max_tile_y = (s16)(aabb->max.y >> 4);
    for (s16 tile_y = min_tile_y; tile_y <= max_tile_y; tile_y++)
    {
        for (s16 tile_x = min_tile_x; tile_x <= max_tile_x; tile_x++)
        {
            const TileCollisionData* tile_collision_data = get_tile_collision_data(level, tile_x, tile_y);
            if (!tile_collision_data || !tile_collision_data->geometry) continue;
            const TileGeometryData* tile_geometry = tile_collision_data->geometry;
            out_scan->geom_flags_hit |= (u16)tile_geometry->flags;
            if (tile_geometry->shape == TILE_SHAPE_HMAP)
                out_scan->saw_hmap = 1;
            if (!out_scan->behavior &&
                tile_collision_data->behavior &&
                tile_collision_data->behavior->type != TILE_BEHAVIOR_NONE)
            {
                out_scan->behavior = tile_collision_data->behavior;
                out_scan->beh_tx = tile_x;
                out_scan->beh_ty = tile_y;
            }
        }
    }
}
const TileBehaviorData* find_overlapping_tile_behavior(
    const Level* level,
    const AABB* aabb,
    TileBehaviorType behavior_type)
{
    if (!level || !aabb) return NULL;
    const s16 cx_tile = (s16)(((aabb->min.x + aabb->max.x) >> 1) >> 4);
    const s16 cy_tile = (s16)(aabb->max.y >> 4);
    decompress_collision_window(level, (u16)cx_tile, (u16)cy_tile);
    const s16 min_tile_x = (s16)(aabb->min.x >> 4);
    const s16 max_tile_x = (s16)(aabb->max.x >> 4);
    const s16 min_tile_y = (s16)(aabb->min.y >> 4);
    const s16 max_tile_y = (s16)(aabb->max.y >> 4);
    for (s16 tile_y = min_tile_y; tile_y <= max_tile_y; tile_y++)
    {
        for (s16 tile_x = min_tile_x; tile_x <= max_tile_x; tile_x++)
        {
            const TileCollisionData* tile = get_tile_collision_data(level, tile_x, tile_y);
            if (!tile || !tile->behavior) continue;
            if (tile->behavior->type == behavior_type)
                return tile->behavior;
        }
    }
    return NULL;
}
void collision_resolve_walls(Level* level, AABB* box, CollisionResult* out)
{
    out->flags &= (u16)~(COLLISION_LEFT | COLLISION_RIGHT);
    const s16 prev_min_x = (s16)(box->min.x - out->vx);
    const s16 prev_max_x = (s16)(box->max.x - out->vx);
    const s16 head_y  = (s16)(box->min.y + 4);
    const s16 upper_y = (s16)(box->min.y + ((box->max.y - box->min.y) >> 2));
    const s16 mid_y   = (s16)((box->min.y + box->max.y) >> 1);
    const s16 feet_y  = (s16)(box->max.y - 2);
    s16 y_probes[4];
    u16 probe_count = 0;
    y_probes[probe_count++] = head_y;
    y_probes[probe_count++] = upper_y;
    y_probes[probe_count++] = mid_y;
    y_probes[probe_count++] = feet_y;
    for (u16 i = 0; i < probe_count; i++)
    {
        if (y_probes[i] < box->min.y) y_probes[i] = box->min.y;
        if (y_probes[i] > box->max.y) y_probes[i] = box->max.y;
    }
    if (out->vx > 0)
    {
        s16 start_x = (s16)(prev_max_x + 1);
        s16 end_x   = (s16)(box->max.x + 1);
        for (s16 probe_x = start_x; probe_x <= end_x; probe_x++)
        {
            const s16 tile_x = (s16)(probe_x >> 4);
            for (u16 i = 0; i < probe_count; i++)
            {
                const s16 probe_y = y_probes[i];
                const s16 tile_y  = (s16)(probe_y >> 4);
                const TileCollisionData* tile = get_tile_collision_data(level, tile_x, tile_y);
                if (!tile || !tile->geometry) continue;
                const TileGeometryData* geom = tile->geometry;
                if (!(geom->flags & TILE_FLAG_BLOCK_LEFT)) continue;
                if (!tile_is_solid_at_world_point(tile, tile_x, tile_y, probe_x, probe_y)) continue;
                const s16 w = (s16)(box->max.x - box->min.x + 1);
                box->max.x = (s16)((tile_x << 4) - 1);
                box->min.x = (s16)(box->max.x - w + 1);
                out->vx = 0;
                out->flags |= COLLISION_RIGHT;
                if (tile->behavior) out->specialType = (u16)tile->behavior->type;
                out->hit_tx = tile_x;
                out->hit_ty = tile_y;
                return;
            }
        }
    }
    if (out->vx < 0)
    {
        s16 start_x = (s16)(prev_min_x - 1);
        s16 end_x   = (s16)(box->min.x - 1);
        for (s16 probe_x = start_x; probe_x >= end_x; probe_x--)
        {
            const s16 tile_x = (s16)(probe_x >> 4);
            for (u16 i = 0; i < probe_count; i++)
            {
                const s16 probe_y = y_probes[i];
                const s16 tile_y  = (s16)(probe_y >> 4);
                const TileCollisionData* tile = get_tile_collision_data(level, tile_x, tile_y);
                if (!tile || !tile->geometry) continue;
                const TileGeometryData* geom = tile->geometry;
                if (!(geom->flags & TILE_FLAG_BLOCK_RIGHT)) continue;
                if (!tile_is_solid_at_world_point(tile, tile_x, tile_y, probe_x, probe_y)) continue;
                const s16 w = (s16)(box->max.x - box->min.x + 1);
                box->min.x = (s16)(((tile_x + 1) << 4));
                box->max.x = (s16)(box->min.x + w - 1);
                out->vx = 0;
                out->flags |= COLLISION_LEFT;
                if (tile->behavior) out->specialType = (u16)tile->behavior->type;
                out->hit_tx = tile_x;
                out->hit_ty = tile_y;
                return;
            }
        }
    }
}
void collision_resolve_ceiling(Level* level, AABB* box, CollisionResult* out)
{
    if (out->vy >= 0) return;
    out->flags &= (u16)~COLLISION_CEILING;
    const s16 head_y = (s16)(box->min.y - 1);
    const s16 head_ty = (s16)(head_y >> 4);
    const s16 center_x  = (s16)((box->min.x + box->max.x) >> 1);
    const s16 center_tx = (s16)(center_x >> 4);
    const TileCollisionData* center_tile = get_tile_collision_data(level, center_tx, head_ty);
    const bool center_blocked =
        center_tile &&
        center_tile->geometry &&
        (center_tile->geometry->flags & TILE_FLAG_BLOCK_UP) &&
        tile_is_solid_at_world_point(center_tile, center_tx, head_ty, center_x, head_y);
    if (center_blocked)
    {
        const s16 h = (s16)(box->max.y - box->min.y + 1);
        box->min.y = (s16)((head_ty + 1) << 4);
        box->max.y = (s16)(box->min.y + h - 1);
        out->vy = 0;
        out->flags |= COLLISION_CEILING;
        if (center_tile->behavior)
            out->specialType = (u16)center_tile->behavior->type;
        out->hit_tx = center_tx;
        out->hit_ty = head_ty;
        return;
    }
    const s16 left_x  = (s16)(box->min.x + 1);
    const s16 right_x = (s16)(box->max.x - 1);
    const s16 left_tx  = (s16)(left_x  >> 4);
    const s16 right_tx = (s16)(right_x >> 4);
    bool left_corner_hit = FALSE;
    bool right_corner_hit = FALSE;
    {
        const TileCollisionData* t = get_tile_collision_data(level, left_tx, head_ty);
        if (t && t->geometry && (t->geometry->flags & TILE_FLAG_BLOCK_UP))
            left_corner_hit = tile_is_solid_at_world_point(t, left_tx, head_ty, left_x, head_y);
    }
    {
        const TileCollisionData* t = get_tile_collision_data(level, right_tx, head_ty);
        if (t && t->geometry && (t->geometry->flags & TILE_FLAG_BLOCK_UP))
            right_corner_hit = tile_is_solid_at_world_point(t, right_tx, head_ty, right_x, head_y);
    }
    if (!left_corner_hit && !right_corner_hit)
        return;
    s16 dir_first = 0;
    if (left_corner_hit && !right_corner_hit) dir_first = +1;
    else if (right_corner_hit && !left_corner_hit) dir_first = -1;
    else dir_first = (out->vx >= 0) ? +1 : -1;
    const s16 dirs[2] = { dir_first, (s16)-dir_first };
    for (u16 d = 0; d < 2; d++)
    {
        const s16 dir = dirs[d];
        for (s16 dx = 1; dx <= CEILING_NUDGE_MAX; dx++)
        {
            const s16 shift = (s16)(dir * dx);
            const s16 test_center_x = (s16)(center_x + shift);
            const s16 test_center_tx = (s16)(test_center_x >> 4);
            const TileCollisionData* t = get_tile_collision_data(level, test_center_tx, head_ty);
            const bool blocked =
                t && t->geometry &&
                (t->geometry->flags & TILE_FLAG_BLOCK_UP) &&
                tile_is_solid_at_world_point(t, test_center_tx, head_ty, test_center_x, head_y);
            if (!blocked)
            {
                box->min.x = (s16)(box->min.x + shift);
                box->max.x = (s16)(box->max.x + shift);
                return;
            }
        }
    }
}
bool collision_resolve_floor(Level* level, AABB* box, CollisionResult* out)
{
    out->flags &= (u16)~COLLISION_FLOOR;
    const s16 h          = (s16)(box->max.y - box->min.y + 1);
    const s16 foot_y     = (s16)(box->max.y + 1);
    const s16 foot_y_m1  = (s16)(box->max.y);
    const s16 feet_ty    = (s16)(foot_y >> 4);
    const s16 support_x  = (s16)(((box->min.x + box->max.x) >> 1) + out->vx);
    const s16 tx         = (s16)(support_x >> 4);
    bool found = FALSE;
    s16 best_surface_y = NO_FLOOR_SURFACE_Y;
    s16 best_ty = 0;
    const TileCollisionData* best_t = NULL;
    for (s16 row_offset = -1; row_offset <= 1; row_offset++)
    {
        const s16 ty = (s16)(feet_ty + row_offset);
        const TileCollisionData* t = get_tile_collision_data(level, tx, ty);
        if (!t || !t->geometry) continue;
        if (out->ignore_one_way && (t->geometry->flags & TILE_FLAG_ONE_WAY)) continue;
        const s16 surface_y = tile_floor_surface_y_at_x(t, tx, ty, support_x);
        if (surface_y == NO_FLOOR_SURFACE_Y) continue;
        const s16 dy0 = (s16)(surface_y - foot_y);
        const s16 dy1 = (s16)(surface_y - foot_y_m1);
        if (out->vy >= 0 &&
            ((surface_y >= foot_y - out->vy && surface_y <= foot_y) ||
             (dy0 >= -ONE_WAY_PLATFORM_ERROR_CORRECTION && dy0 <= MAX_STEP_DOWN) ||
            (dy1 >= -ONE_WAY_PLATFORM_ERROR_CORRECTION && dy1 <= MAX_STEP_DOWN)))
        {
            if (!found || surface_y < best_surface_y)
            {
                found = TRUE;
                best_surface_y = surface_y;
                best_ty = ty;
                best_t = t;
            }
        }
        if (out->vy == 0 && out->vx != 0)
        {
            if ((dy0 < 0 && (-dy0) <= MAX_STEP_UP) || (dy1 < 0 && (-dy1) <= MAX_STEP_UP))
            {
                if (!found || surface_y < best_surface_y)
                {
                    found = TRUE; best_surface_y = surface_y; best_ty = ty; best_t = t;
                }
            }
        }
    }
    if (!found) return FALSE;
    box->max.y = (s16)(best_surface_y - 1);
    box->min.y = (s16)(box->max.y - h + 1);
    out->vy = 0;
    out->flags |= COLLISION_FLOOR;
    if (best_t && best_t->behavior) out->specialType = (u16)best_t->behavior->type;
    out->hit_tx = tx;
    out->hit_ty = best_ty;
    return TRUE;
}
void resolve_level_collisions(Level* level, AABB* box, CollisionResult* cr)
{
    cr->flags = 0;
    cr->specialType = 0;
    s16 cx_tile = (s16)(((box->min.x + box->max.x) >> 1) >> 4);
    s16 cy_tile = (s16)(box->max.y >> 4);
    decompress_collision_window(level, (u16)cx_tile, (u16)cy_tile);
    CollisionTileScanResult scan;
    scan_overlapping_tiles_for_horizontal(level, box, cr->vx, &scan);
    if (scan.behavior)
    {
        cr->specialType = (u16)scan.behavior->type;
        cr->hit_tx = scan.beh_tx;
        cr->hit_ty = scan.beh_ty;
    }
    collision_resolve_walls(level, box, cr);
    cx_tile = (s16)(((box->min.x + box->max.x) >> 1) >> 4);
    cy_tile = (s16)(box->max.y >> 4);
    decompress_collision_window(level, (u16)cx_tile, (u16)cy_tile);
    collision_resolve_ceiling(level, box, cr);
    cx_tile = (((box->min.x + box->max.x) >> 1) >> 4);
    cy_tile = (box->max.y >> 4);
    decompress_collision_window(level, (u16)cx_tile, (u16)cy_tile);
    collision_resolve_floor(level, box, cr);
}

void collision_bind(const uint8_t *cells, uint16_t width, uint16_t height,
                    const TileCollisionData *const *tiles, uint16_t count) {
    active_level = (Level){cells, width, height, count, tiles};
    player_collision_result = (CollisionResult){0};
}
static AABB player_box(s16 x, s16 y, s16 height) {
    return (AABB){{x,y},{x + PLAYER_WIDTH - 1,y + height - 1}};
}
bool collision_blocked(int16_t x, int16_t y) {
    AABB box = player_box(x,y,playerBody.height ? playerBody.height : PLAYER_HEIGHT);
    if (x < 0 || box.max.x >= active_level.map_tile_width * 16 || y < 0) return true;
    /* Legacy initializer probe: floor contact, without geometry penetration. */
    CollisionResult cr = {.vy=0};
    return collision_aabb_overlaps_solid(&active_level,&box,true)
        || collision_resolve_floor(&active_level,&box,&cr);
}
bool collision_can_stand(int16_t x, int16_t y) {
    AABB head = {{x,y},{x + PLAYER_WIDTH - 1,y + PLAYER_HEIGHT - PLAYER_CROUCH_HEIGHT - 1}};
    return y >= 0 && !collision_aabb_overlaps_solid(&active_level,&head,true);
}
bool collision_can_drop(void) {
    AABB box = player_box(playerBody.x,playerBody.y,playerBody.height);
    return collision_aabb_floor_is_only_one_way_or_empty(&active_level,&box);
}
static void clamp_player_box(AABB *box, CollisionResult *cr) {
    s16 width = box->max.x - box->min.x, height = box->max.y - box->min.y;
    if (box->min.x < 0) { box->min.x=0; box->max.x=width; cr->flags |= COLLISION_LEFT; }
    if (box->max.x >= active_level.map_tile_width*16) {
        box->max.x=active_level.map_tile_width*16-1; box->min.x=box->max.x-width; cr->flags |= COLLISION_RIGHT;
    }
    if (box->min.y < 0) { box->min.y=0; box->max.y=height; cr->flags |= COLLISION_CEILING; }
}
void collision_move_player(int16_t dx, int16_t dy) {
    AABB box = player_box(playerBody.x+dx,playerBody.y+dy,playerBody.height);
    CollisionResult cr = {.vx=dx,.vy=dy,.ignore_one_way=playerBody.drop_timer>0};
    resolve_level_collisions(&active_level,&box,&cr);
    clamp_player_box(&box,&cr);
    playerBody.x=box.min.x; playerBody.y=box.min.y;
    if (cr.flags & (COLLISION_LEFT|COLLISION_RIGHT)) playerBody.fixX=playerBody.velocity_x=0;
    if (cr.flags & (COLLISION_CEILING|COLLISION_FLOOR)) playerBody.fixY=0;
    playerBody.onGround=(cr.flags & COLLISION_FLOOR)!=0;
    player_collision_result=cr;
}
const TileBehaviorData *collision_player_behavior(TileBehaviorType type) {
    AABB box=player_box(playerBody.x,playerBody.y,playerBody.height);
    return find_overlapping_tile_behavior(&active_level,&box,type);
}
