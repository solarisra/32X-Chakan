#ifndef CHAKAN_SH_HUB_SPRITES_H
#define CHAKAN_SH_HUB_SPRITES_H
#include "gdl/gdl.h"
void hub_sprites_init(void);
void hub_sprites_submit(GdlFrame *frame, uint32_t tick);
int16_t hub_portal_overlap(int16_t x,int16_t y,uint16_t width,uint16_t height);
#endif
