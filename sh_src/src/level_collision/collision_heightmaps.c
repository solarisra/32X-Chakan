#include "level_collision/collision_heightmaps.h"
/* From Chakan_Remake_v1 collision_heightmaps.c and te1_collision_tiles. */
const uint8_t te1_collision_heightmaps[10][16] = {
    {0},
    {16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16},
    {16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16},
    {8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8},
    {1,1,2,2,3,3,4,4,5,5,6,6,7,7,8,8},
    {9,9,10,10,11,11,12,12,13,13,14,14,15,15,16,16},
    {16,16,15,15,14,14,13,13,12,12,11,11,10,10,9,9},
    {8,8,7,7,6,6,5,5,4,4,3,3,2,2,1,1},
    {16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16},
    {8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8}
};

static const TileGeometryData empty = {TILE_SHAPE_EMPTY,0,NULL};
static const TileGeometryData solid = {TILE_SHAPE_FULL,FLAGS_SOLID,NULL};
static const TileGeometryData floors[8] = {
    {TILE_SHAPE_HMAP,FLAGS_FLOOR,te1_collision_heightmaps[2]},
    {TILE_SHAPE_HMAP,FLAGS_FLOOR,te1_collision_heightmaps[3]},
    {TILE_SHAPE_HMAP,FLAGS_FLOOR,te1_collision_heightmaps[4]},
    {TILE_SHAPE_HMAP,FLAGS_FLOOR,te1_collision_heightmaps[5]},
    {TILE_SHAPE_HMAP,FLAGS_FLOOR,te1_collision_heightmaps[6]},
    {TILE_SHAPE_HMAP,FLAGS_FLOOR,te1_collision_heightmaps[7]},
    {TILE_SHAPE_HMAP,FLAGS_FLOOR|TILE_FLAG_ONE_WAY,te1_collision_heightmaps[8]},
    {TILE_SHAPE_HMAP,FLAGS_FLOOR|TILE_FLAG_ONE_WAY,te1_collision_heightmaps[9]}
};
static const TileCollisionData earth_tiles[10] = {
    {&empty,NULL},{&solid,NULL},{&floors[0],NULL},{&floors[1],NULL},
    {&floors[2],NULL},{&floors[3],NULL},{&floors[4],NULL},{&floors[5],NULL},
    {&floors[6],NULL},{&floors[7],NULL}
};
const TileCollisionData *const te1_collision_tiles[10] = {
    &earth_tiles[0],&earth_tiles[1],&earth_tiles[2],&earth_tiles[3],&earth_tiles[4],
    &earth_tiles[5],&earth_tiles[6],&earth_tiles[7],&earth_tiles[8],&earth_tiles[9]
};
static const TileBehaviorData hub_doors[5] = {
    {TILE_BEHAVIOR_DOOR,1,0},{TILE_BEHAVIOR_DOOR,2,0},
    {TILE_BEHAVIOR_DOOR,3,0},{TILE_BEHAVIOR_DOOR,4,0},{TILE_BEHAVIOR_DOOR,5,0}
};
static const TileCollisionData hub_tiles[5] = {
    {&empty,&hub_doors[0]},{&empty,&hub_doors[1]},{&empty,&hub_doors[2]},
    {&empty,&hub_doors[3]},{&empty,&hub_doors[4]}
};
const TileCollisionData *const portal_hub_collision_tiles[7] = {
    &earth_tiles[0],&earth_tiles[1],&hub_tiles[0],&hub_tiles[1],&hub_tiles[2],&hub_tiles[3],&hub_tiles[4]
};
/* Air tile 2 is a nine-pixel solid heightmap, not a one-way platform. */
static const uint8_t air_9h[16] = {9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9};
static const TileGeometryData air_9h_geometry = {TILE_SHAPE_HMAP, FLAGS_SOLID, air_9h};
static const TileCollisionData air_9h_tile = {&air_9h_geometry, NULL};
const TileCollisionData *const ta1_collision_tiles[5] = {
    &earth_tiles[0], &earth_tiles[1], &air_9h_tile, &earth_tiles[1], &earth_tiles[0]
};

/* Water 1 geometry, from create_level_stages collision_heightmaps.c. */
static const uint8_t water_9h[16]={9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9};
static const uint8_t water_14hl[16]={16,16,16,16,16,16,16,16,16,16,16,16,16,16,0,0};
static const TileGeometryData water_9h_geometry={TILE_SHAPE_HMAP,FLAGS_FLOOR,water_9h};
static const TileGeometryData water_14hl_geometry={TILE_SHAPE_HMAP,FLAGS_FLOOR,water_14hl};
static const TileCollisionData water_9h_tile={&water_9h_geometry,NULL};
static const TileCollisionData water_14hl_tile={&water_14hl_geometry,NULL};
static const u8 HMAP_TW1_COLLISION_TILE_03[16] = {10, 10, 10, 10, 10, 10, 10, 10, 10, 9, 9, 8, 8, 7, 7, 7};
static const u8 HMAP_TW1_COLLISION_TILE_04[16] = {6, 5, 4, 4, 3, 2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0};
static const u8 HMAP_TW1_COLLISION_TILE_05[16] = {16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 15, 14, 12, 11, 10, 8};
static const u8 HMAP_TW1_COLLISION_TILE_06[16] = { 9,  8,  7,  6,  4,  3,  2,  1,  0,  0,  0,  0,  0,  0,  0,  0};
static const u8 HMAP_TW1_COLLISION_TILE_07[16] = {16, 16, 16, 16, 16, 16, 16, 16, 16, 15, 14, 14, 13, 12, 11, 10};
static const u8 HMAP_TW1_COLLISION_TILE_08[16] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 15, 16, 16, 16, 16, 16};
static const u8 HMAP_TW1_COLLISION_TILE_09[16] = {16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 15, 15, 15, 15};
static const u8 HMAP_TW1_COLLISION_TILE_10[16] = {15, 15, 15, 15, 15, 15, 15, 15, 15, 14, 14, 14, 13, 13, 12, 11};
static const u8 HMAP_TW1_COLLISION_TILE_11[16] = {11, 10, 10, 10, 9, 9, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
static const u8 HMAP_TW1_COLLISION_TILE_12[16] = {0, 2, 6, 8, 9, 10, 13, 15, 15, 15, 15, 15, 15, 15, 15, 15};
static const u8 HMAP_TW1_COLLISION_TILE_14[16] = {15, 15, 15, 15, 15, 15, 15, 15, 14, 14, 12, 9, 7, 6, 4, 1};
static const u8 HMAP_TW1_COLLISION_TILE_15[16] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 9, 9, 10, 10, 10, 11};
static const u8 HMAP_TW1_COLLISION_TILE_16[16] = {11, 12, 13, 13, 14, 14, 14, 15, 15, 15, 15, 15, 15, 15, 15, 15};
static const u8 HMAP_TW1_COLLISION_TILE_17[16] = {15, 15, 15, 15, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16};
static const u8 HMAP_TW1_COLLISION_TILE_18[16] = {16, 16, 16, 16, 16, 15, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
static const u8 HMAP_TW1_COLLISION_TILE_19[16] = {1, 4, 6, 7, 9, 12, 14, 15, 15, 15, 15, 15, 15, 15, 15, 15};
static const u8 HMAP_TW1_COLLISION_TILE_20[16] = {15, 15, 15, 15, 15, 15, 15, 15, 15, 13, 10, 9, 8, 6, 2, 0};
static const u8 HMAP_TW1_COLLISION_TILE_21[16] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 3, 4, 6, 7, 8, 8};
static const u8 HMAP_TW1_COLLISION_TILE_22[16] = {10, 11, 12, 13, 14, 14, 15, 16, 16, 16, 16, 16, 16, 16, 16, 16};
static const u8 HMAP_TW1_COLLISION_TILE_23[16] = {8, 10, 11, 12, 14, 15, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16};
static const u8 HMAP_TW1_COLLISION_TILE_24[16] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 3, 4, 6, 7, 8, 9};
static const u8 HMAP_TW1_COLLISION_TILE_25[16] = {7, 7, 7, 8, 8, 9, 9, 10, 10, 10, 10, 10, 10, 10, 10, 10};
static const TileGeometryData GEOM_tw1_collision_tile_03 = {TILE_SHAPE_HMAP, FLAGS_FLOOR, HMAP_TW1_COLLISION_TILE_03};
static const TileGeometryData GEOM_tw1_collision_tile_04 = {TILE_SHAPE_HMAP, FLAGS_FLOOR, HMAP_TW1_COLLISION_TILE_04};
static const TileGeometryData GEOM_tw1_collision_tile_05 = {TILE_SHAPE_HMAP, FLAGS_FLOOR, HMAP_TW1_COLLISION_TILE_05};
static const TileGeometryData GEOM_tw1_collision_tile_06 = {TILE_SHAPE_HMAP, FLAGS_FLOOR, HMAP_TW1_COLLISION_TILE_06};
static const TileGeometryData GEOM_tw1_collision_tile_07 = {TILE_SHAPE_HMAP, FLAGS_FLOOR, HMAP_TW1_COLLISION_TILE_07};
static const TileGeometryData GEOM_tw1_collision_tile_08 = {TILE_SHAPE_HMAP, FLAGS_FLOOR | TILE_FLAG_ONE_WAY, HMAP_TW1_COLLISION_TILE_08};
static const TileGeometryData GEOM_tw1_collision_tile_09 = {TILE_SHAPE_HMAP, FLAGS_FLOOR | TILE_FLAG_ONE_WAY, HMAP_TW1_COLLISION_TILE_09};
static const TileGeometryData GEOM_tw1_collision_tile_10 = {TILE_SHAPE_HMAP, FLAGS_FLOOR | TILE_FLAG_ONE_WAY, HMAP_TW1_COLLISION_TILE_10};
static const TileGeometryData GEOM_tw1_collision_tile_11 = {TILE_SHAPE_HMAP, FLAGS_FLOOR | TILE_FLAG_ONE_WAY, HMAP_TW1_COLLISION_TILE_11};
static const TileGeometryData GEOM_tw1_collision_tile_12 = {TILE_SHAPE_HMAP, FLAGS_FLOOR, HMAP_TW1_COLLISION_TILE_12};
static const TileGeometryData GEOM_tw1_collision_tile_14 = {TILE_SHAPE_HMAP, FLAGS_FLOOR, HMAP_TW1_COLLISION_TILE_14};
static const TileGeometryData GEOM_tw1_collision_tile_15 = {TILE_SHAPE_HMAP, FLAGS_FLOOR | TILE_FLAG_ONE_WAY, HMAP_TW1_COLLISION_TILE_15};
static const TileGeometryData GEOM_tw1_collision_tile_16 = {TILE_SHAPE_HMAP, FLAGS_FLOOR | TILE_FLAG_ONE_WAY, HMAP_TW1_COLLISION_TILE_16};
static const TileGeometryData GEOM_tw1_collision_tile_17 = {TILE_SHAPE_HMAP, FLAGS_FLOOR | TILE_FLAG_ONE_WAY, HMAP_TW1_COLLISION_TILE_17};
static const TileGeometryData GEOM_tw1_collision_tile_18 = {TILE_SHAPE_HMAP, FLAGS_FLOOR | TILE_FLAG_ONE_WAY, HMAP_TW1_COLLISION_TILE_18};
static const TileGeometryData GEOM_tw1_collision_tile_19 = {TILE_SHAPE_HMAP, FLAGS_FLOOR, HMAP_TW1_COLLISION_TILE_19};
static const TileGeometryData GEOM_tw1_collision_tile_20 = {TILE_SHAPE_HMAP, FLAGS_FLOOR, HMAP_TW1_COLLISION_TILE_20};
static const TileGeometryData GEOM_tw1_collision_tile_21 = {TILE_SHAPE_HMAP, FLAGS_FLOOR, HMAP_TW1_COLLISION_TILE_21};
static const TileGeometryData GEOM_tw1_collision_tile_22 = {TILE_SHAPE_HMAP, FLAGS_FLOOR, HMAP_TW1_COLLISION_TILE_22};
static const TileGeometryData GEOM_tw1_collision_tile_23 = {TILE_SHAPE_HMAP, FLAGS_FLOOR, HMAP_TW1_COLLISION_TILE_23};
static const TileGeometryData GEOM_tw1_collision_tile_24 = {TILE_SHAPE_HMAP, FLAGS_FLOOR, HMAP_TW1_COLLISION_TILE_24};
static const TileGeometryData GEOM_tw1_collision_tile_25 = {TILE_SHAPE_HMAP, FLAGS_FLOOR, HMAP_TW1_COLLISION_TILE_25};
const TileCollisionData tw1_collision_tile_03 = {&GEOM_tw1_collision_tile_03, NULL};
const TileCollisionData tw1_collision_tile_04 = {&GEOM_tw1_collision_tile_04, NULL};
const TileCollisionData tw1_collision_tile_05 = {&GEOM_tw1_collision_tile_05, NULL};
const TileCollisionData tw1_collision_tile_06 = {&GEOM_tw1_collision_tile_06, NULL};
const TileCollisionData tw1_collision_tile_07 = {&GEOM_tw1_collision_tile_07, NULL};
const TileCollisionData tw1_collision_tile_08 = {&GEOM_tw1_collision_tile_08, NULL};
const TileCollisionData tw1_collision_tile_09 = {&GEOM_tw1_collision_tile_09, NULL};
const TileCollisionData tw1_collision_tile_10 = {&GEOM_tw1_collision_tile_10, NULL};
const TileCollisionData tw1_collision_tile_11 = {&GEOM_tw1_collision_tile_11, NULL};
const TileCollisionData tw1_collision_tile_12 = {&GEOM_tw1_collision_tile_12, NULL};
const TileCollisionData tw1_collision_tile_14 = {&GEOM_tw1_collision_tile_14, NULL};
const TileCollisionData tw1_collision_tile_15 = {&GEOM_tw1_collision_tile_15, NULL};
const TileCollisionData tw1_collision_tile_16 = {&GEOM_tw1_collision_tile_16, NULL};
const TileCollisionData tw1_collision_tile_17 = {&GEOM_tw1_collision_tile_17, NULL};
const TileCollisionData tw1_collision_tile_18 = {&GEOM_tw1_collision_tile_18, NULL};
const TileCollisionData tw1_collision_tile_19 = {&GEOM_tw1_collision_tile_19, NULL};
const TileCollisionData tw1_collision_tile_20 = {&GEOM_tw1_collision_tile_20, NULL};
const TileCollisionData tw1_collision_tile_21 = {&GEOM_tw1_collision_tile_21, NULL};
const TileCollisionData tw1_collision_tile_22 = {&GEOM_tw1_collision_tile_22, NULL};
const TileCollisionData tw1_collision_tile_23 = {&GEOM_tw1_collision_tile_23, NULL};
const TileCollisionData tw1_collision_tile_24 = {&GEOM_tw1_collision_tile_24, NULL};
const TileCollisionData tw1_collision_tile_25 = {&GEOM_tw1_collision_tile_25, NULL};

const TileCollisionData *const tw1_collision_tiles[27]={
    &earth_tiles[0],
    &earth_tiles[1],
    &water_9h_tile,
    &tw1_collision_tile_03,
    &tw1_collision_tile_04,
    &tw1_collision_tile_05,
    &tw1_collision_tile_06,
    &tw1_collision_tile_07,
    &tw1_collision_tile_08,
    &tw1_collision_tile_09,
    &tw1_collision_tile_10,
    &tw1_collision_tile_11,
    &tw1_collision_tile_12,
    &water_14hl_tile,
    &tw1_collision_tile_14,
    &tw1_collision_tile_15,
    &tw1_collision_tile_16,
    &tw1_collision_tile_17,
    &tw1_collision_tile_18,
    &tw1_collision_tile_19,
    &tw1_collision_tile_20,
    &tw1_collision_tile_21,
    &tw1_collision_tile_22,
    &tw1_collision_tile_23,
    &tw1_collision_tile_24,
    &tw1_collision_tile_25,
    &earth_tiles[2]
};

/* Fire 1: source 8h floors, three one-way profiles, and solid bottom seal. */
static const uint8_t fire_right_8h[16]={0,0,0,0,0,0,0,0,8,8,8,8,8,8,8,8};
static const uint8_t fire_left_8h[16]={8,8,8,8,8,8,8,8,0,0,0,0,0,0,0,0};
static const TileGeometryData fire_platform_geometry[3]={
    {TILE_SHAPE_HMAP,FLAGS_FLOOR|TILE_FLAG_ONE_WAY,te1_collision_heightmaps[3]},
    {TILE_SHAPE_HMAP,FLAGS_FLOOR|TILE_FLAG_ONE_WAY,fire_right_8h},
    {TILE_SHAPE_HMAP,FLAGS_FLOOR|TILE_FLAG_ONE_WAY,fire_left_8h}
};
static const TileCollisionData fire_platforms[3]={
    {&fire_platform_geometry[0],NULL}, {&fire_platform_geometry[1],NULL},
    {&fire_platform_geometry[2],NULL}
};
const TileCollisionData *const tf1_collision_tiles[8]={
    &earth_tiles[0],&earth_tiles[1],&earth_tiles[3],&fire_platforms[0],
    &fire_platforms[1],&fire_platforms[2],&earth_tiles[3],&earth_tiles[1]
};
