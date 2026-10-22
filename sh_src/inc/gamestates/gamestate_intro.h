#ifndef CHAKAN_GAMESTATE_INTRO_H
#define CHAKAN_GAMESTATE_INTRO_H
#include "gdl/gdl.h"
void gamestate_intro_init(uint16_t pad);
bool gamestate_intro_update(uint16_t pad);
const GdlFrame *gamestate_intro_frame(void);
void gamestate_intro_presented(void);
void gamestate_intro_lightning(void); /* Master/68000 service callback. */
#endif
