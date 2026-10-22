#include "gdl/video.h"
#include "mars.h"

static uint16_t pending_colors[256], current_colors[256], dirty_first = 256, dirty_end;
static uint16_t fade_colors[80];

void gdl_palette_begin_fade(void) {
    for (uint16_t i=0;i<80;i++) fade_colors[i]=current_colors[i];
}
void gdl_palette_fade_to_black(uint16_t step,uint16_t duration) {
    if (!duration || step>duration) return;
    uint16_t colors[80], remaining=duration-step;
    for (uint16_t i=0;i<80;i++) {
        uint16_t c=fade_colors[i];
        colors[i]=((c&31)*remaining/duration)
            | ((((c>>5)&31)*remaining/duration)<<5)
            | ((((c>>10)&31)*remaining/duration)<<10);
    }
    gdl_palette_schedule(0,colors,80);
}

void gdl_video_init(void) {
    dirty_first = 256;
    dirty_end = 0;
    Hw32xInit(MARS_VDP_MODE_256, 0);
    MARS_VDP_DISPMODE |= MARS_VDP_PRIO_32X;
}
void gdl_palette_load(uint16_t first, const uint16_t *colors, uint16_t count) {
    if (first >= 256 || count > 256 - first || !colors) return;
    /* PEN alone permits HBlank writes. CRAM access during active scanout can
     * produce colored dots; commit the whole range during vertical blank. */
    while (!(MARS_VDP_FBCTL & MARS_VDP_VBLK)) {}
    volatile uint16_t *palette = &MARS_CRAM;
    for (uint16_t i = 0; i < count; i++) {
        while (!(MARS_VDP_FBCTL & MARS_VDP_PEN)) {}
        palette[first + i] = colors[i];
        current_colors[first + i] = colors[i];
    }
}
GdlSurface gdl_video_backbuffer(void) {
    GdlSurface surface = {(volatile uint32_t *)(&MARS_FRAMEBUFFER + 0x100), 80};
    return surface;
}
void gdl_palette_schedule(uint16_t first, const uint16_t *colors, uint16_t count) {
    if (first >= 256 || count > 256 - first || !colors || !count) return;
    for (uint16_t i = 0; i < count; i++) pending_colors[first + i] = colors[i];
    /* Fill gaps from the current palette to preserve separately queued ranges. */
    if (dirty_first == 256) { dirty_first = first; dirty_end = first + count; }
    else {
        if (first > dirty_end) for (uint16_t i = dirty_end; i < first; i++) pending_colors[i] = current_colors[i];
        if (first + count < dirty_first) for (uint16_t i = first + count; i < dirty_first; i++) pending_colors[i] = current_colors[i];
        if (first < dirty_first) dirty_first = first;
        if (first + count > dirty_end) dirty_end = first + count;
    }
}
void gdl_video_present(void) {
    Hw32xScreenFlip(1);
    if (dirty_first < dirty_end) {
        gdl_palette_load(dirty_first, pending_colors + dirty_first, dirty_end - dirty_first);
        dirty_first = 256;
        dirty_end = 0;
    }
}
