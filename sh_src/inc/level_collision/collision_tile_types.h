/* Portable types from create_level_stages. */
#pragma once
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
typedef uint8_t u8;
typedef uint16_t u16;
typedef int16_t s16;
#define TRUE true
#define FALSE false
#define TILE_HMAP_W 16
typedef enum TileShape {
    TILE_SHAPE_EMPTY = 0,
    TILE_SHAPE_FULL = 1,
    TILE_SHAPE_HMAP = 2
} TileShape;
typedef enum TileGeometryFlags {
    TILE_FLAG_BLOCK_DOWN = 1 << 0,
    TILE_FLAG_BLOCK_UP = 1 << 1,
    TILE_FLAG_BLOCK_LEFT = 1 << 2,
    TILE_FLAG_BLOCK_RIGHT = 1 << 3,
    TILE_FLAG_ONE_WAY = 1 << 4
} TileGeometryFlags;
#define FLAGS_FLOOR (TILE_FLAG_BLOCK_DOWN)
#define FLAGS_SOLID (TILE_FLAG_BLOCK_DOWN | TILE_FLAG_BLOCK_UP | TILE_FLAG_BLOCK_LEFT | TILE_FLAG_BLOCK_RIGHT)
typedef struct TileGeometryData {
    TileShape shape;            
    TileGeometryFlags flags;    
    const u8 *hmap;             
} TileGeometryData;
typedef enum TileBehaviorType {
    TILE_BEHAVIOR_NONE = 0,
    TILE_BEHAVIOR_HAZARD = 1, // Causes damage or negative effects
    TILE_BEHAVIOR_SWITCH = 2, // Can be interacted with to trigger events
    TILE_BEHAVIOR_TRIGGER = 3, // Triggers scripts/events when stepped on
    TILE_BEHAVIOR_DOOR = 4, // Teleports player or triggers level transition
    TILE_BEHAVIOR_LADDER = 5, // Allows climbing ladders
    TILE_BEHAVIOR_ROPE = 6, // Allows climbing ropes
    TILE_BEHAVIOR_WATER = 7, // Water tile (swimming, drowning, etc.)
    TILE_BEHAVIOR_ICE = 8, // Slippery tile
    TILE_BEHAVIOR_CURRENT = 9, // Tile with directional current
} TileBehaviorType;
typedef struct TileBehaviorData {
    TileBehaviorType type;   
    u8 id;     
    u8 param;   
} TileBehaviorData;
typedef struct TileCollisionData {
    const TileGeometryData *geometry;   
    const TileBehaviorData *behavior;       
} TileCollisionData;
