#ifndef CHAKAN_PLAYER_H
#define CHAKAN_PLAYER_H

#include <stdint.h>
#include <stdbool.h>

/* SGDK fix16 has six fractional bits. Keep its rounding for negative motion. */
#define PLAYER_FIXED_ONE 64
#define PLAYER_WIDTH 42
#define PLAYER_HEIGHT 58
#define PLAYER_CROUCH_HEIGHT 40
#define PLAYER_OFFSET_X 35
#define PLAYER_OFFSET_Y 22

typedef struct {
    int16_t x, y;
    int16_t fixX, fixY;
    int16_t velocity_x;
    int16_t input_x, input_y;
    int16_t height;
    int16_t facingDirection;
    bool onGround, jumping;
    bool crouching, rolling;
    uint8_t drop_timer;
} Player;

typedef bool (*PlayerBlocked)(int16_t x, int16_t y);
/* Optional level collision resolver; updates position, collision velocities
 * and onGround. A fresh playerInit clears it for legacy solid-cell levels. */
typedef void (*PlayerMove)(int16_t dx, int16_t dy);
void player_set_collision_move(PlayerMove move);
void player_set_collision_queries(bool (*can_drop)(void), bool (*can_stand)(int16_t, int16_t));
bool player_request_drop_through(void);
bool player_request_roll(void);
void player_set_jump_held(bool held);
extern Player playerBody;
extern const uint16_t coyoteTime;
extern uint16_t currentCoyoteTime;
extern const int16_t jumpBufferTime;
extern int16_t currentJumpBufferTime;

int16_t player_fixed_to_int(int16_t value);
void playerInit(int16_t x, int16_t y, PlayerBlocked blocked);
void updatePlayer(void);
void player_set_control_enabled(bool enabled);
bool player_is_control_enabled(void);
void player_set_simulation_enabled(bool enabled);
bool player_is_simulation_enabled(void);

#endif
