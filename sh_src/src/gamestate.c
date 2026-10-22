#include "gamestate.h"
#include "gamestates/gamestate_logo.h"
#include "gamestates/gamestate_intro.h"
#include "level_manager.h"
#include "pause_menu.h"
#include "input_controls.h"
#include "gamestates/gamestate_menu.h"
GAME_STATE currentState;
static bool paused;
static uint16_t play_previous;
void gamestate_init(void) {
    paused=false;play_previous=0;
    currentState=STATE_LOGO;
    gamestate_logo_init();
    gamestate_changed(currentState);
}
void gamestate_update(uint16_t pad) {
    if (currentState==STATE_LOGO) {
        if (gamestate_logo_update()) {
            gamestate_intro_init(pad);
            currentState=STATE_INTRO;
            gamestate_changed(currentState);
        }
    } else if (currentState==STATE_INTRO) {
        if (gamestate_intro_update(pad)) {
            gamestate_menu_init(pad); currentState=STATE_MENU; gamestate_changed(currentState);
        }
    } else if (currentState==STATE_MENU) {
        if (gamestate_menu_update(pad)) {
            level_manager_init(); play_previous=pad; paused=false; currentState=STATE_PLAY; gamestate_changed(currentState);
        }
    } else if (currentState==STATE_PLAY) {
        uint16_t pressed=pad&~play_previous;play_previous=pad;
        if(paused) {
            if(pause_menu_update(pad)) {pause_menu_close(pad);paused=false;}
        } else if((pressed&BUTTON_START) && (pad&BUTTON_RETURN_HUB)!=BUTTON_RETURN_HUB) {
            pause_menu_open(pad);paused=true;
        } else level_manager_update(pad);
    }
}
const GdlFrame *gamestate_render_frame(void) {
    if (currentState==STATE_INTRO) return gamestate_intro_frame();
    if (currentState==STATE_MENU) return gamestate_menu_frame();
    return currentState==STATE_LOGO?gamestate_logo_frame():level_manager_render_frame();
}

void gamestate_frame_presented(void) {
    if (currentState==STATE_INTRO) gamestate_intro_presented();
    else if (currentState==STATE_MENU) gamestate_menu_presented();
}

bool gamestate_is_paused(void) {return currentState==STATE_PLAY && paused;}
const GdlFrame *gamestate_prepare_frame(GdlFrame *scratch) {
    if(gamestate_is_paused()) {pause_menu_frame(scratch);return scratch;}
    return gamestate_render_frame();
}
