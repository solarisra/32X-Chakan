#include "gdl/video.h"
#include "mars.h"

void gdl_video_init(void) {
    Hw32xInit(MARS_VDP_MODE_256, 0);
    MARS_VDP_DISPMODE |= MARS_VDP_PRIO_32X;
}
void gdl_palette_load(uint16_t first, const uint16_t *colors, uint16_t count) {
    if (first >= 256 || count > 256 - first || !colors) return;
    volatile uint16_t *palette = &MARS_CRAM;
    for (uint16_t i = 0; i < count; i++) palette[first + i] = colors[i];
}
GdlSurface gdl_video_backbuffer(void) {
    GdlSurface surface = {(volatile uint32_t *)(&MARS_FRAMEBUFFER + 0x100), 80};
    return surface;
}
void gdl_video_present(void) { Hw32xScreenFlip(1); }
