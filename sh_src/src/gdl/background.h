#ifndef GDL_BACKGROUND_H
#define GDL_BACKGROUND_H
#include <stdint.h>
/* SH-2 has no variable barrel shift. Keep pixel alignment shifts constant
 * rather than emitting shift helper calls inside every tile group. */
static inline void background_align(const uint32_t *a,const uint32_t *b,uint16_t phase,
                                   uint32_t *left,uint32_t *right) {
    switch (phase) {
    case 0: *left=a[0]; *right=a[1]; break;
    case 1: *left=(a[0]<<8)|(a[1]>>24); *right=(a[1]<<8)|(b[0]>>24); break;
    case 2: *left=(a[0]<<16)|(a[1]>>16); *right=(a[1]<<16)|(b[0]>>16); break;
    case 3: *left=(a[0]<<24)|(a[1]>>8); *right=(a[1]<<24)|(b[0]>>8); break;
    case 4: *left=a[1]; *right=b[0]; break;
    case 5: *left=(a[1]<<8)|(b[0]>>24); *right=(b[0]<<8)|(b[1]>>24); break;
    case 6: *left=(a[1]<<16)|(b[0]>>16); *right=(b[0]<<16)|(b[1]>>16); break;
    default: *left=(a[1]<<24)|(b[0]>>8); *right=(b[0]<<24)|(b[1]>>8); break;
    }
}
/* Align eight background pixels to a foreground tile's scratch-buffer origin.
 * The two layers may have different sub-tile offsets. */
static inline void background_pixels(const uint32_t *tiles, const uint16_t *map,
        uint16_t tile_x, uint16_t mask, uint16_t row, uint16_t phase,
        uint32_t *left, uint32_t *right) {
    const uint32_t *a = tiles + map[tile_x & mask] + row;
    if (!phase) { *left=a[0]; *right=a[1]; return; }
    const uint32_t *b = tiles + map[(tile_x+1) & mask] + row;
    background_align(a,b,phase,left,right);
}
/* Compact backgrounds index an eight-entry directory per tile. */
static inline void background_pixels_compact(const uint32_t *tiles, const uint16_t *map,
        const uint16_t *rows, uint16_t tile_x, uint16_t mask, uint16_t row, uint16_t phase,
        uint32_t *left, uint32_t *right) {
    const uint32_t *a=tiles+rows[map[tile_x&mask]+row];
    if (!phase) { *left=a[0]; *right=a[1]; return; }
    const uint32_t *b=tiles+rows[map[(tile_x+1)&mask]+row];
    background_align(a,b,phase,left,right);
}
#endif
