#ifndef CHAKAN_GAMESTATE_MENU_H
#define CHAKAN_GAMESTATE_MENU_H
#include "gdl/gdl.h"
typedef enum {MENU_SPLASH,SPLASH_START,MAIN_MENU,OPTIONS_MENU} MenuState;
extern MenuState currentMenuState;
extern uint8_t menuSelection;
void gamestate_menu_init(uint16_t pad);
bool gamestate_menu_update(uint16_t pad);
const GdlFrame *gamestate_menu_frame(void);
void gamestate_menu_presented(void);
#endif
