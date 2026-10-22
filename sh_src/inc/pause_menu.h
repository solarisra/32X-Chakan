#ifndef CHAKAN_PAUSE_MENU_H
#define CHAKAN_PAUSE_MENU_H
#include "gdl/gdl.h"
void pause_menu_open(uint16_t pad);
void pause_menu_close(uint16_t pad);
bool pause_menu_update(uint16_t pad); /* true requests resume */
void pause_menu_frame(GdlFrame *scratch);
void gamestate_play_menu_changed(bool open); /* Master/68000 callback */
#endif
