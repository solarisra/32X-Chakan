#include "gamestate.h"
#include "gamestates/gamestate_logo.h"
#include "gamestates/gamestate_intro.h"
#include "level_manager.h"
#include "gamestates/gamestate_menu.h"
GAME_STATE currentState;
void gamestate_init(void) {
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
            level_manager_init(); currentState=STATE_PLAY; gamestate_changed(currentState);
        }
    } else if (currentState==STATE_PLAY) level_manager_update(pad);
}
const GdlFrame *gamestate_render_frame(void) {
    if (currentState==STATE_INTRO) return gamestate_intro_frame();
    if (currentState==STATE_MENU) return gamestate_menu_frame();
    return currentState==STATE_LOGO?gamestate_logo_frame():level_manager_render_frame();
}

void gamestate_frame_presented(void) {
    if (currentState==STATE_INTRO) gamestate_intro_presented();
}
