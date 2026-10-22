#ifndef GDL_VIDEO_H
#define GDL_VIDEO_H
#include "renderer.h"
/* Master SH-2 only. Current backend is 320x224 indexed 8-bit. */
void gdl_video_init(void);
void gdl_palette_load(uint16_t first, const uint16_t *colors, uint16_t count);
/* Master queues changed colors; applied at the next completed VBlank flip. */
void gdl_palette_schedule(uint16_t first, const uint16_t *colors, uint16_t count);
/* Capture the game's first 80 displayed colors; fade from that fixed snapshot. */
void gdl_palette_begin_fade(void);
void gdl_palette_fade_to_black(uint16_t step, uint16_t duration);
GdlSurface gdl_video_backbuffer(void);
void gdl_video_present(void);
#endif
