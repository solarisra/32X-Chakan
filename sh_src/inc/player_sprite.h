#ifndef CHAKAN_SH_PLAYER_SPRITE_H
#define CHAKAN_SH_PLAYER_SPRITE_H
#include "gdl/renderer.h"
void player_sprite_init(void);
void player_sprite_submit(GdlFrame *frame, const GdlSpanImage *image, int16_t camera_x, int16_t camera_y);
#endif
