/* Movement adapted from Chakan_Remake_v1/src/player.c.
 * Rendering and solid-cell collision are supplied by the level for this slice.
 */
#include "player.h"

#define SPEED 2
#define JUMP_SPEED 12
#define MAX_FALL_SPEED 12
#define ACCELERATION 16  /* FIX16(.25) */
#define DECELERATION 12  /* FIX16(.2), truncated as in SGDK */
#define GRAVITY 32       /* FIX16(.5), original gravityScale */

Player playerBody;
const uint16_t coyoteTime = 5;
uint16_t currentCoyoteTime;
const int16_t jumpBufferTime = 5;
int16_t currentJumpBufferTime;
static bool control_enabled = true;
static bool simulation_enabled = true;
static PlayerBlocked level_blocked;

int16_t player_fixed_to_int(int16_t value) {
    /* Explicit floor division, independent of host signed-right-shift behavior. */
    if (value < 0) return -(int16_t)((-(int32_t)value + 63) / 64);
    return value / 64;
}

void playerInit(int16_t x, int16_t y, PlayerBlocked blocked) {
    playerBody = (Player){0};
    playerBody.x = x;
    playerBody.y = y;
    playerBody.facingDirection = 1;
    level_blocked = blocked;
    playerBody.onGround = blocked(x, y + 1);
    currentCoyoteTime = playerBody.onGround ? coyoteTime : 0;
    currentJumpBufferTime = 0;
    control_enabled = true;
    simulation_enabled = true;
}

void player_set_control_enabled(bool enabled) {
    control_enabled = enabled;
    if (!enabled) {
        playerBody.input_x = 0;
        currentJumpBufferTime = 0;
    }
}
bool player_is_control_enabled(void) { return control_enabled; }
void player_set_simulation_enabled(bool enabled) {
    simulation_enabled = enabled;
    if (!enabled) {
        playerBody.input_x = 0;
        playerBody.fixX = playerBody.fixY = playerBody.velocity_x = 0;
        currentJumpBufferTime = 0;
    }
}
bool player_is_simulation_enabled(void) { return simulation_enabled; }

void updatePlayer(void) {
    if (!simulation_enabled) return;
    bool did_jump = currentCoyoteTime > 0 && currentJumpBufferTime > 0;
    if (did_jump) {
        playerBody.jumping = true;
        playerBody.fixY = -JUMP_SPEED * PLAYER_FIXED_ONE;
        currentCoyoteTime = 0;
        currentJumpBufferTime = 0;
    }
    if (currentJumpBufferTime > 0) currentJumpBufferTime--;

    if (playerBody.input_x > 0) {
        if (playerBody.velocity_x != SPEED) playerBody.fixX += ACCELERATION;
        playerBody.facingDirection = 1;
    } else if (playerBody.input_x < 0) {
        if (playerBody.velocity_x != -SPEED) playerBody.fixX -= ACCELERATION;
        playerBody.facingDirection = -1;
    } else if (playerBody.onGround) {
        if (playerBody.velocity_x > 0) playerBody.fixX -= DECELERATION;
        else if (playerBody.velocity_x < 0) playerBody.fixX += DECELERATION;
        else playerBody.fixX = 0;
    }
    playerBody.velocity_x = player_fixed_to_int(playerBody.fixX);
    if (playerBody.velocity_x > SPEED) playerBody.velocity_x = SPEED;
    if (playerBody.velocity_x < -SPEED) playerBody.velocity_x = -SPEED;

    /* Preserve original gravity gating and fixed-point vertical velocity. */
    if (!playerBody.onGround) {
        if (player_fixed_to_int(playerBody.fixY) <= MAX_FALL_SPEED)
            playerBody.fixY += GRAVITY;
        else playerBody.fixY = MAX_FALL_SPEED * PLAYER_FIXED_ONE;
    }

    int16_t step = playerBody.velocity_x < 0 ? -1 : 1;
    int16_t distance = playerBody.velocity_x * step;
    for (int16_t i = 0; i < distance; i++) {
        if (level_blocked(playerBody.x + step, playerBody.y)) {
            playerBody.fixX = playerBody.velocity_x = 0;
            break;
        }
        playerBody.x += step;
    }
    int16_t vy = player_fixed_to_int(playerBody.fixY);
    step = vy < 0 ? -1 : 1;
    distance = vy * step;
    for (int16_t i = 0; i < distance; i++) {
        if (level_blocked(playerBody.x, playerBody.y + step)) {
            playerBody.fixY = 0;
            break;
        }
        playerBody.y += step;
    }
    playerBody.onGround = vy >= 0 && level_blocked(playerBody.x, playerBody.y + 1);
    if (playerBody.onGround) {
        currentCoyoteTime = coyoteTime;
        if (!did_jump) playerBody.jumping = false;
        playerBody.fixY = 0;
    } else if (currentCoyoteTime > 0) currentCoyoteTime--;
}
