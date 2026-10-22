#ifndef CHAKAN_SH_GAMESTATE_H
#define CHAKAN_SH_GAMESTATE_H
#include "gdl/gdl.h"
typedef enum {STATE_LOGO,STATE_INTRO,STATE_MENU,STATE_PLAY,STATE_PAUSE} GAME_STATE;
extern GAME_STATE currentState;
void gamestate_init(void);
void gamestate_update(uint16_t pad);
const GdlFrame *gamestate_render_frame(void);
void gamestate_frame_presented(void);
bool gamestate_is_paused(void);
const GdlFrame *gamestate_prepare_frame(GdlFrame *scratch);
void gamestate_changed(GAME_STATE state); /* Master/68000 service callback. */
#endif
