#ifndef CHAKAN_GAMESTATE_MENU_H
#define CHAKAN_GAMESTATE_MENU_H
#include "gdl/gdl.h"
typedef enum {MENU_SPLASH,SPLASH_START,MAIN_MENU,OPTIONS_MENU} MenuState;
extern MenuState currentMenuState;
extern uint8_t menuSelection;
void gamestate_menu_init(uint16_t pad);
void gamestate_menu_letter(void); /* Master/68000 service callback. */
bool gamestate_menu_update(uint16_t pad);
const GdlFrame *gamestate_menu_frame(void);
#endif
