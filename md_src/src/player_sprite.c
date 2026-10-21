/* Genesis SAT renderer for the original Chakan sprite sheet. */
#include <stdint.h>
#include "common.h"
#include "player.h"
#include "player_sprite.h"
#include "sprites.h"

static uint16_t animation, frame, ticks;
static volatile uint16_t *const data = (uint16_t *)VDP_DATA_PORT;
static volatile uint32_t *const control = (uint32_t *)VDP_CTRL_PORT;

void player_sprite_init(void) {
    animation = frame = ticks = 0;
    *control = ((uint32_t)(0x4000 | ((CHAKAN_TILE_BASE * 32) & 0x3FFF)) << 16)
        | ((CHAKAN_TILE_BASE * 32) >> 14);
    for (uint16_t i = 0; i < CHAKAN_TILE_COUNT * 16; i++) *data = chakan_tiles[i];
    *control = 0xC0400000; /* Palette 2. */
    for (uint16_t i = 0; i < 16; i++) *data = chakan_palette[i];
    player_sprite_update();
}

void player_sprite_update(void) {
    uint16_t next = playerBody.onGround ? (playerBody.velocity_x ? 1 : 0)
        : (playerBody.fixY < 0 ? 2 : 3);
    if (playerBody.onGround && animation == 3) next = 4;
    if (animation == 4 && playerBody.onGround && ticks < 8) next = 4;
    if (next != animation) {
        animation = next;
        frame = ticks = 0;
    } else if (++ticks >= 8 && animation == 1) {
        frame = (frame + 1) & 7;
        ticks = 0;
    }
    uint16_t index = animation == 0 ? 0 : animation == 1 ? 1 + frame : animation + 7;
    uint16_t start = chakan_frame_offsets[index];
    uint16_t end = chakan_frame_offsets[index + 1];
    bool flip = playerBody.facingDirection > 0;
    *control = 0x7C000003; /* SAT at 0xFC00. */
    for (uint16_t i = start; i < end; i++) {
        int16_t x = flip ? 104 - chakan_pieces[i].x : chakan_pieces[i].x;
        *data = (playerBody.y - PLAYER_OFFSET_Y + chakan_pieces[i].y + 128) & 0x3FF;
        *data = i + 1 < end ? i - start + 1 : 0; /* 8x8, linked SAT entry. */
        *data = 0xC000 | (flip ? 0x0800 : 0) | (CHAKAN_TILE_BASE + chakan_pieces[i].tile);
        *data = (playerBody.x - PLAYER_OFFSET_X + x + 128) & 0x1FF;
    }
}
