#ifndef CHAKAN_SH_GAMESTATE_LOGO_H
#define CHAKAN_SH_GAMESTATE_LOGO_H
#include "gdl/gdl.h"
void gamestate_logo_init(void);
bool gamestate_logo_update(void);
const GdlFrame *gamestate_logo_frame(void);
void gamestate_logo_scream(void); /* Master/68000 service callback. */
#endif
