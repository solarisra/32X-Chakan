#ifndef GDL_VIDEO_H
#define GDL_VIDEO_H
#include "renderer.h"
/* Master SH-2 only. Current backend is 320x224 indexed 8-bit. */
void gdl_video_init(void);
void gdl_palette_load(uint16_t first, const uint16_t *colors, uint16_t count);
GdlSurface gdl_video_backbuffer(void);
void gdl_video_present(void);
#endif
