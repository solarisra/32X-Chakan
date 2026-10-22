#include <assert.h>
#include <stdio.h>
#include "player.h"
#include "input_controls.h"

static bool floor_enabled;
static bool wall_enabled;
static bool ceiling_enabled;
static bool blocked(int16_t x, int16_t y) {
    return (floor_enabled && y + PLAYER_HEIGHT > 160)
        || (wall_enabled && x + PLAYER_WIDTH > 100)
        || (ceiling_enabled && y < 40);
}
static void reset(void) {
    floor_enabled = true;
    wall_enabled = false;
    ceiling_enabled = false;
    playerInit(40, 160 - PLAYER_HEIGHT, blocked);
    input_init();
}
static void frame(uint16_t pad) {
    processInput(pad);
    updatePlayer();
}
static void land(uint16_t pad) {
    for (int i = 0; i < 120 && !playerBody.onGround; i++) frame(pad);
    assert(playerBody.onGround);
}

int main(void) {
    /* Original acceleration: 1/4 pixel steps, integer displacement at 1px. */
    reset();
    for (int i = 0; i < 3; i++) frame(BUTTON_RIGHT);
    assert(playerBody.x == 40);
    frame(BUTTON_RIGHT);
    assert(playerBody.x == 41);
    for (int i = 0; i < 4; i++) frame(BUTTON_RIGHT);
    assert(playerBody.velocity_x == 2);
    for (int i = 0; i < 20; i++) frame(0);
    assert(playerBody.fixX == 0 && playerBody.velocity_x == 0);
    frame(BUTTON_LEFT | BUTTON_RIGHT);
    assert(playerBody.input_x == 1);

    /* A held button cannot repeatedly jump on landing. */
    reset();
    frame(BUTTON_A);
    assert(playerBody.jumping && playerBody.fixY == -12 * 64);
    assert(playerBody.y == 160 - PLAYER_HEIGHT - 12);
    land(BUTTON_A);
    for (int i = 0; i < 20; i++) frame(BUTTON_A);
    assert(playerBody.onGround && playerBody.y == 160 - PLAYER_HEIGHT);
    frame(0);
    frame(BUTTON_C);
    assert(playerBody.fixY < 0);

    /* Releasing the final jump button halves upward velocity exactly once. */
    reset();
    frame(BUTTON_B);
    processInput(0);
    assert(playerBody.fixY == -6 * 64);
    processInput(0);
    assert(playerBody.fixY == -6 * 64);
    updatePlayer();
    assert(playerBody.fixY == -352);

    /* Coyote window expires after five updates without support. */
    reset();
    floor_enabled = false;
    frame(0);
    assert(currentCoyoteTime == 4);
    frame(BUTTON_A);
    assert(playerBody.jumping && playerBody.fixY < 0);
    reset();
    floor_enabled = false;
    for (int i = 0; i < 5; i++) frame(0);
    frame(BUTTON_A);
    assert(!playerBody.jumping && playerBody.fixY >= 0);

    /* A jump pressed just before landing is consumed on the next frame. */
    reset();
    playerBody.y = 160 - PLAYER_HEIGHT - 4;
    playerBody.onGround = false;
    currentCoyoteTime = 0;
    playerBody.fixY = 4 * 64;
    frame(BUTTON_A);
    assert(playerBody.onGround && currentJumpBufferTime == 4);
    frame(BUTTON_A);
    assert(playerBody.jumping && playerBody.fixY < 0);
    reset();
    floor_enabled = false;
    currentCoyoteTime = 0;
    frame(BUTTON_A);
    for (int i = 0; i < 5; i++) frame(BUTTON_A);
    assert(currentJumpBufferTime == 0 && !playerBody.jumping);

    /* Axis collisions clear velocity; large impulses cannot tunnel. */
    reset();
    wall_enabled = true;
    playerBody.x = 58;
    for (int i = 0; i < 12; i++) frame(BUTTON_RIGHT);
    assert(playerBody.x == 58 && playerBody.fixX == 0);
    reset();
    ceiling_enabled = true;
    playerBody.y = 45;
    playerBody.onGround = false;
    playerBody.fixY = -12 * 64;
    frame(0);
    assert(playerBody.y == 40 && playerBody.fixY == 0);

    /* Ground friction is disabled in air; gravity stays near its 12px cap. */
    reset();
    floor_enabled = false;
    playerBody.onGround = false;
    playerBody.fixX = 128;
    playerBody.velocity_x = 2;
    for (int i = 0; i < 60; i++) {
        frame(0);
        assert(playerBody.fixY <= 13 * 64);
    }
    assert(playerBody.fixX == 128 && playerBody.velocity_x == 2);

    reset();
    player_set_control_enabled(false);
    frame(BUTTON_RIGHT | BUTTON_A);
    assert(playerBody.input_x == 0 && !playerBody.jumping);
    player_set_simulation_enabled(false);
    int16_t y = playerBody.y;
    floor_enabled = false;
    for (int i = 0; i < 10; i++) frame(0);
    assert(playerBody.y == y && playerBody.fixY == 0);
    player_set_simulation_enabled(true);
    frame(0);
    frame(0);
    assert(playerBody.fixY > 0);

    puts("Player/input behavior checks passed");
    return 0;
}
