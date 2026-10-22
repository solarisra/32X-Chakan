/* Movement/actions adapted from Chakan_Remake_v1 create_level_stages.
 * The level supplies movement resolution, headroom and drop eligibility.
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
static PlayerMove level_move;
static bool (*level_can_drop)(void);
static bool (*level_can_stand)(int16_t,int16_t);
static bool jump_held, forced_crouch;
static int16_t roll_direction;
static uint16_t roll_remaining;
void player_set_collision_queries(bool (*drop)(void), bool (*stand)(int16_t,int16_t)) {
    level_can_drop=drop; level_can_stand=stand;
}
static void stop_roll(void) {
    playerBody.rolling=false; roll_remaining=0;
    playerBody.fixX=playerBody.velocity_x=0;
}
void player_set_jump_held(bool held) { jump_held=held; if (!held && playerBody.rolling) stop_roll(); }
static bool input_crouch(void) {
    return playerBody.input_y>0 && !playerBody.velocity_x && !playerBody.fixY && playerBody.onGround;
}
bool player_request_drop_through(void) {
    if (!control_enabled || !simulation_enabled || !playerBody.onGround || !level_can_drop || !level_can_drop()) return false;
    playerBody.drop_timer=12; playerBody.onGround=playerBody.jumping=false;
    currentCoyoteTime=currentJumpBufferTime=0;
    if (playerBody.fixY < PLAYER_FIXED_ONE) playerBody.fixY=PLAYER_FIXED_ONE;
    return true;
}
bool player_request_roll(void) {
    if (!control_enabled || !simulation_enabled || playerBody.rolling || !playerBody.onGround
        || (!playerBody.crouching && playerBody.input_y<=0)) return false;
    playerBody.rolling=true; roll_direction=playerBody.facingDirection>0?1:-1;
    roll_remaining=137; jump_held=true; currentJumpBufferTime=0;
    return true;
}
void player_set_collision_move(PlayerMove move) { level_move = move; }

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
    playerBody.height = PLAYER_HEIGHT;
    level_can_drop=0; level_can_stand=0;
    jump_held=forced_crouch=false; roll_remaining=0;
    level_blocked = blocked;
    level_move = 0;
    playerBody.onGround = blocked(x, y + 1);
    currentCoyoteTime = playerBody.onGround ? coyoteTime : 0;
    currentJumpBufferTime = 0;
    control_enabled = true;
    simulation_enabled = true;
}

void player_set_control_enabled(bool enabled) {
    control_enabled = enabled;
    if (!enabled) {
        playerBody.input_x = playerBody.input_y = 0;
        playerBody.drop_timer=0; jump_held=forced_crouch=false; stop_roll();
        currentJumpBufferTime = 0;
    }
}
bool player_is_control_enabled(void) { return control_enabled; }
void player_set_simulation_enabled(bool enabled) {
    simulation_enabled = enabled;
    if (!enabled) {
        playerBody.input_x = playerBody.input_y = 0;
        playerBody.drop_timer=0; jump_held=forced_crouch=false; stop_roll();
        playerBody.fixX = playerBody.fixY = playerBody.velocity_x = 0;
        currentJumpBufferTime = 0;
    }
}
bool player_is_simulation_enabled(void) { return simulation_enabled; }

void updatePlayer(void) {
    if (!simulation_enabled) return;
    int16_t previous_x=playerBody.x;
    bool did_jump = currentCoyoteTime > 0 && currentJumpBufferTime > 0;
    if (did_jump) {
        playerBody.jumping = true;
        playerBody.fixY = -JUMP_SPEED * PLAYER_FIXED_ONE;
        currentCoyoteTime = 0;
        currentJumpBufferTime = 0;
    }
    if (currentJumpBufferTime > 0) currentJumpBufferTime--;

    playerBody.crouching=forced_crouch || input_crouch();
    if (playerBody.rolling) {
        if (!jump_held || !roll_remaining) stop_roll();
        else playerBody.fixX=roll_direction * (roll_remaining<8?roll_remaining:8) * PLAYER_FIXED_ONE;
    } else if (playerBody.crouching) {
        playerBody.fixX=playerBody.velocity_x=0;
    } else if (playerBody.input_x > 0) {
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
    int16_t max_speed=playerBody.rolling?8:SPEED;
    if (playerBody.velocity_x > max_speed) playerBody.velocity_x = max_speed;
    if (playerBody.velocity_x < -max_speed) playerBody.velocity_x = -max_speed;

    /* Preserve original gravity gating and fixed-point vertical velocity. */
    if (!playerBody.onGround) {
        if (player_fixed_to_int(playerBody.fixY) <= MAX_FALL_SPEED)
            playerBody.fixY += GRAVITY;
        else playerBody.fixY = MAX_FALL_SPEED * PLAYER_FIXED_ONE;
    }

    int16_t vy = player_fixed_to_int(playerBody.fixY);
    bool short_box=input_crouch() || playerBody.rolling;
    if (!short_box && playerBody.height==PLAYER_CROUCH_HEIGHT && level_can_stand) {
        forced_crouch=!level_can_stand(playerBody.x+playerBody.velocity_x,
            playerBody.y+playerBody.height-PLAYER_HEIGHT+vy);
        short_box=forced_crouch;
    } else forced_crouch=false;
    int16_t height=short_box?PLAYER_CROUCH_HEIGHT:PLAYER_HEIGHT;
    playerBody.y+=playerBody.height-height;
    playerBody.height=height;
    playerBody.crouching=forced_crouch || input_crouch();
    if (level_move) {
        level_move(playerBody.velocity_x, vy);
    } else {
        int16_t step = playerBody.velocity_x < 0 ? -1 : 1;
        int16_t distance = playerBody.velocity_x * step;
        for (int16_t i = 0; i < distance; i++) {
            if (level_blocked(playerBody.x + step, playerBody.y)) {
                playerBody.fixX = playerBody.velocity_x = 0;
                break;
            }
            playerBody.x += step;
        }
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
    }
    if (playerBody.drop_timer) playerBody.drop_timer--;
    if (playerBody.rolling) {
        int16_t distance=playerBody.x-previous_x;
        if (distance<0) distance=-distance;
        if (distance>=roll_remaining) stop_roll();
        else roll_remaining-=distance;
    }
    playerBody.crouching=forced_crouch || input_crouch();
    if (playerBody.onGround) {
        currentCoyoteTime = coyoteTime;
        if (!did_jump) playerBody.jumping = false;
        playerBody.fixY = 0;
    } else if (currentCoyoteTime > 0) currentCoyoteTime--;
}
