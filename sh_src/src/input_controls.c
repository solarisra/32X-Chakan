/* Gameplay input adapted from Chakan_Remake_v1/src/input_controls.c. */
#include "input_controls.h"
#include "player.h"

uint16_t prevJoyInput;

void input_init(void) {
    prevJoyInput = 0;
}

void processInput(uint16_t joyState) {
    joyHandlerStatePlay(joyState ^ prevJoyInput, joyState);
    prevJoyInput = joyState;
}

void joyHandlerStatePlay(uint16_t changed, uint16_t state) {
    if (!player_is_control_enabled() || !player_is_simulation_enabled()) {
        playerBody.input_x = playerBody.input_y = 0;
        return;
    }
    playerBody.input_y=(state & BUTTON_DOWN)?1:(state & BUTTON_UP)?-1:0;
    /* Preserve the original right-button priority for opposite directions. */
    if (state & BUTTON_RIGHT) playerBody.input_x = 1;
    else if (state & BUTTON_LEFT) playerBody.input_x = -1;
    else playerBody.input_x = 0;

    if (changed & BUTTON_JUMP) {
        if (state & BUTTON_JUMP) {
            player_set_jump_held(true);
            if ((state & BUTTON_DOWN) && player_request_drop_through()) currentJumpBufferTime=0;
            else if (player_request_roll()) currentJumpBufferTime=0;
            else currentJumpBufferTime=jumpBufferTime;
        }
        else {
            player_set_jump_held(false);
            if (playerBody.jumping && playerBody.fixY < 0) {
                /* SGDK F16_mul(v, FIX16(.5)) rounds down for negative values. */
                playerBody.fixY = player_fixed_to_int(playerBody.fixY * 32);
            }
        }
    }
}
