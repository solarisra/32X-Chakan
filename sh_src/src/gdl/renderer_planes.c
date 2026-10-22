/* General two-plane priority composition used by Water's raster effect.
 * Other scenes retain the specialized renderer paths. */
#include "gdl/renderer.h"
#include "background.h"
static bool priority_at(const uint8_t *bits,uint32_t tile) {
    return bits && (bits[tile>>3] & (1u<<(tile&7)));
}
static uint32_t opaque_pixels(uint32_t pixels) {
    uint32_t p=pixels&0x0F0F0F0FUL;
    return ((p|(p>>1)|(p>>2)|(p>>3))&0x01010101UL)*255;
}
static uint32_t priority_word(bool first,bool second,uint16_t count) {
    if (first==second) return first?0xFFFFFFFFUL:0;
    static const uint32_t prefix[5]={0,0xFF000000UL,0xFFFF0000UL,0xFFFFFF00UL,0xFFFFFFFFUL};
    return (first?prefix[count]:0)|(second?~prefix[count]:0);
}
static void render_priority_rows(const GdlScene *scene, const GdlFrame *frame,
                     const GdlSurface *surface, uint16_t first, uint16_t end) {
    int16_t camera_x = frame->camera_x, camera_y = frame->camera_y;
    uint16_t star_y = frame->background_y;
    const uint32_t *fg_tiles = scene->foreground.tiles, *bg_tiles = scene->background.tiles;
    const uint16_t *fg_row_offsets = scene->foreground.tile_rows;
    const uint16_t *fg_map = scene->foreground.map, *bg_map = scene->background.map;
    uint16_t fg_width = scene->foreground.width_tiles;
    uint16_t bg_width = scene->background.width_tiles;
    uint16_t bg_x_mask = bg_width - 1;
    uint16_t bg_height = scene->background.height_tiles * 8;

    if (!surface || !surface->pixels || surface->pitch_words < GDL_WIDTH / 4) return;
    if (first > GDL_HEIGHT) first = GDL_HEIGHT;
    if (end > GDL_HEIGHT) end = GDL_HEIGHT;
    uint16_t by=(first+star_y)%bg_height;
    for (uint16_t y = first; y < end; y++) {
        uint32_t words[82];
        uint32_t coverage[82], priority_coverage[82];
        bool occlusion = false;
        for (uint16_t si = 0; si < frame->sprite_count; si++) {
            const GdlSprite *sprite = &frame->sprites[si];
            int32_t sy = (int32_t)y - sprite->y;
            if (sprite->behind_foreground && sy >= 0 && sy < sprite->image->height) {
                occlusion = true; break;
            }
        }
#if __BYTE_ORDER__ != __ORDER_LITTLE_ENDIAN__
        uint8_t *line = (uint8_t *)words;
#endif
        uint16_t fy = y + camera_y;
        const uint16_t *fm = fg_map + (fy >> 3) * fg_width;
        const uint16_t *bm = bg_map + (by >> 3) * bg_width;
        uint16_t offset = camera_x & 7;
        uint16_t tile_x = camera_x >> 3;
        uint16_t scroll_x = frame->background_per_line ? frame->background_rows[y] :
            frame->background_per_row ? frame->background_rows[y / 8] : frame->background_x;
        uint16_t bg_x = (scroll_x - offset) & (bg_width * 8 - 1);
        uint16_t bg_first = bg_x >> 3, bg_phase = bg_x & 7;
        uint16_t columns = (320 + offset + 7) >> 3;
        for (uint16_t column = 0; column < columns; column++, tile_x++) {
            uint16_t entry=fm[tile_x];
            const uint32_t *fg = fg_tiles + (fg_row_offsets
                ? fg_row_offsets[(entry & (frame->foreground_map_priority?32767:65535)) + (fy & 7)]
                : entry + (fy & 7) * 5);
            uint32_t fg_index=(uint32_t)(fy>>3)*fg_width+tile_x;
            bool fg_priority=frame->foreground_map_priority?(entry&32768)!=0:priority_at(frame->foreground_priority,fg_index);
            uint32_t bg_left=0, bg_right=0, high_left=0, high_right=0;
            if (fg[4]!=1 || !fg_priority) {
                if (scene->background.tile_rows)
                    background_pixels_compact(bg_tiles,bm,scene->background.tile_rows,
                        bg_first+column,bg_x_mask,by&7,bg_phase,&bg_left,&bg_right);
                else background_pixels(bg_tiles,bm,bg_first+column,bg_x_mask,
                        (by&7)*2,bg_phase,&bg_left,&bg_right);
                if (frame->background_pixel_priority) {
                    high_left=((bg_left&0x80808080UL)>>7)*255;
                    high_right=((bg_right&0x80808080UL)>>7)*255;
                    bg_left&=0x7F7F7F7FUL;
                    bg_right&=0x7F7F7F7FUL;
                } else {
                    uint32_t bg_index=(uint32_t)(by>>3)*bg_width;
                    bool p=priority_at(frame->background_priority,bg_index+((bg_first+column)&bg_x_mask));
                    bool q=priority_at(frame->background_priority,bg_index+((bg_first+column+1)&bg_x_mask));
                    uint16_t first_count=8-bg_phase;
                    high_left=priority_word(p,q,first_count>=4?4:first_count)&opaque_pixels(bg_left);
                    high_right=priority_word(p,q,first_count>=4?first_count-4:0)&opaque_pixels(bg_right);
                }
            }
            uint32_t *output=words+column*2;
            output[0]=fg[0]|(bg_left&fg[2]); output[1]=fg[1]|(bg_right&fg[3]);
            if (!fg_priority) {
                output[0]=(output[0]&~high_left)|(bg_left&high_left);
                output[1]=(output[1]&~high_right)|(bg_right&high_right);
            }
            if (occlusion) {
                coverage[column*2]=fg[2]&~high_left;
                coverage[column*2+1]=fg[3]&~high_right;
                priority_coverage[column*2]=~(high_left|(fg_priority?~fg[2]:0));
                priority_coverage[column*2+1]=~(high_right|(fg_priority?~fg[3]:0));
            }
        }
        for (uint16_t si = 0; si < frame->sprite_count; si++) {
            const GdlSprite *sprite = &frame->sprites[si];
            const GdlSpanImage *image = sprite->image;
            const uint32_t *mask=sprite->behind_foreground==GDL_SPRITE_BEHIND_PRIORITY?priority_coverage:coverage;
            int32_t sy = (int32_t)y - sprite->y;
            if (sy < 0 || sy >= image->height) continue;
            uint32_t row_index = (sprite->frame + (sprite->flip ? image->frames : 0)) * image->height + sy;
            for (uint16_t span = image->row_offsets[row_index]; span < image->row_offsets[row_index + 1]; span++) {
                const uint16_t *run = image->spans + span * 3;
                int32_t left = (int32_t)sprite->x + run[0];
                int32_t right = left + run[1];
                if (right <= 0 || left >= GDL_WIDTH) continue;
                const uint8_t *pixels = image->pixels + run[2];
                if (left < 0) { pixels -= left; left = 0; }
                if (right > GDL_WIDTH) right = GDL_WIDTH;
                for (int16_t x = left; x < right; x++) {
                    if (sprite->behind_foreground) {
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
                        uint16_t pixel=x+offset;
                        if (!((mask[pixel>>2]>>((3-(pixel&3))*8))&255)) { pixels++; continue; }
#else
                        if (!((const uint8_t *)mask)[x+offset]) { pixels++; continue; }
#endif
                    }

#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
                    /* Host tests use the same canonical big-endian pixel words. */
                    uint16_t pixel = x + offset;
                    uint16_t shift = (3 - (pixel & 3)) * 8;
                    words[pixel >> 2] = (words[pixel >> 2] & ~(255UL << shift)) | ((uint32_t)*pixels++ << shift);
#else
                    line[x + offset] = *pixels++;
#endif
                }
            }
        }
        const uint32_t *source = words + (offset >> 2);
        volatile uint32_t *destination = surface->pixels + y * surface->pitch_words;
        switch (offset & 3) {
        case 0:
            #pragma GCC unroll 8
            for (uint16_t x = 0; x < 80; x++) destination[x] = source[x];
            break;
        case 1:
            #pragma GCC unroll 8
            for (uint16_t x = 0; x < 80; x++)
                destination[x] = (source[x] << 8) | (source[x + 1] >> 24);
            break;
        case 2:
            #pragma GCC unroll 8
            for (uint16_t x = 0; x < 80; x++)
                destination[x] = (source[x] << 16) | (source[x + 1] >> 16);
            break;
        case 3:
            #pragma GCC unroll 8
            for (uint16_t x = 0; x < 80; x++)
                destination[x] = (source[x] << 24) | (source[x + 1] >> 8);
            break;
        }
        if (++by == bg_height) by=0;
    }
}

static void render_opaque_rows(const GdlScene *scene, const GdlFrame *frame,
                     const GdlSurface *surface, uint16_t first, uint16_t end) {
    int16_t camera_x = frame->camera_x, camera_y = frame->camera_y;
    uint16_t star_y = frame->background_y;
    const uint32_t *fg_tiles = scene->foreground.tiles, *bg_tiles = scene->background.tiles;
    const uint16_t *fg_row_offsets = scene->foreground.tile_rows;
    const uint16_t *fg_map = scene->foreground.map, *bg_map = scene->background.map;
    uint16_t fg_width = scene->foreground.width_tiles;
    uint16_t bg_width = scene->background.width_tiles;
    uint16_t bg_x_mask = bg_width - 1;
    uint16_t bg_height = scene->background.height_tiles * 8;

    if (!surface || !surface->pixels || surface->pitch_words < GDL_WIDTH / 4) return;
    if (first > GDL_HEIGHT) first = GDL_HEIGHT;
    if (end > GDL_HEIGHT) end = GDL_HEIGHT;
    uint16_t by=(first+star_y)%bg_height;
    for (uint16_t y = first; y < end; y++) {
        uint32_t words[82];

#if __BYTE_ORDER__ != __ORDER_LITTLE_ENDIAN__
        uint8_t *line = (uint8_t *)words;
#endif
        uint16_t fy = y + camera_y;
        const uint16_t *fm = fg_map + (fy >> 3) * fg_width;
        const uint16_t *bm = bg_map + (by >> 3) * bg_width;
        uint16_t offset = camera_x & 7;
        uint16_t tile_x = camera_x >> 3;
        uint16_t scroll_x = frame->background_per_line ? frame->background_rows[y] :
            frame->background_per_row ? frame->background_rows[y / 8] : frame->background_x;
        uint16_t bg_x = (scroll_x - offset) & (bg_width * 8 - 1);
        uint16_t bg_first = bg_x >> 3, bg_phase = bg_x & 7;
        uint16_t columns = (320 + offset + 7) >> 3;
        for (uint16_t column = 0; column < columns; column++, tile_x++) {
            const uint32_t *fg = fg_tiles + (fg_row_offsets
                ? fg_row_offsets[(fm[tile_x] & (frame->foreground_map_priority?32767:65535)) + (fy & 7)]
                : fm[tile_x] + (fy & 7) * 5);
            uint32_t bg_left,bg_right;
            if (scene->background.tile_rows)
                background_pixels_compact(bg_tiles,bm,scene->background.tile_rows,
                    bg_first+column,bg_x_mask,by&7,bg_phase,&bg_left,&bg_right);
            else background_pixels(bg_tiles,bm,bg_first+column,bg_x_mask,(by&7)*2,bg_phase,&bg_left,&bg_right);
            if (frame->background_pixel_priority) {
                bg_left&=0x7F7F7F7FUL;
                bg_right&=0x7F7F7F7FUL;
            }
            uint32_t *output=words+column*2;
            if (frame->foreground_map_priority?(fm[tile_x]&32768)!=0:
                priority_at(frame->foreground_priority,(uint32_t)(fy>>3)*fg_width+tile_x)) {
                output[0]=fg[0]|(bg_left&fg[2]); output[1]=fg[1]|(bg_right&fg[3]);
            } else { output[0]=bg_left; output[1]=bg_right; }
        }
        for (uint16_t si = 0; si < frame->sprite_count; si++) {
            const GdlSprite *sprite = &frame->sprites[si];
            const GdlSpanImage *image = sprite->image;
            if (sprite->behind_foreground) continue;
            int32_t sy = (int32_t)y - sprite->y;
            if (sy < 0 || sy >= image->height) continue;
            uint32_t row_index = (sprite->frame + (sprite->flip ? image->frames : 0)) * image->height + sy;
            for (uint16_t span = image->row_offsets[row_index]; span < image->row_offsets[row_index + 1]; span++) {
                const uint16_t *run = image->spans + span * 3;
                int32_t left = (int32_t)sprite->x + run[0];
                int32_t right = left + run[1];
                if (right <= 0 || left >= GDL_WIDTH) continue;
                const uint8_t *pixels = image->pixels + run[2];
                if (left < 0) { pixels -= left; left = 0; }
                if (right > GDL_WIDTH) right = GDL_WIDTH;
                for (int16_t x = left; x < right; x++) {

#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
                    /* Host tests use the same canonical big-endian pixel words. */
                    uint16_t pixel = x + offset;
                    uint16_t shift = (3 - (pixel & 3)) * 8;
                    words[pixel >> 2] = (words[pixel >> 2] & ~(255UL << shift)) | ((uint32_t)*pixels++ << shift);
#else
                    line[x + offset] = *pixels++;
#endif
                }
            }
        }
        const uint32_t *source = words + (offset >> 2);
        volatile uint32_t *destination = surface->pixels + y * surface->pitch_words;
        switch (offset & 3) {
        case 0:
            #pragma GCC unroll 8
            for (uint16_t x = 0; x < 80; x++) destination[x] = source[x];
            break;
        case 1:
            #pragma GCC unroll 8
            for (uint16_t x = 0; x < 80; x++)
                destination[x] = (source[x] << 8) | (source[x + 1] >> 24);
            break;
        case 2:
            #pragma GCC unroll 8
            for (uint16_t x = 0; x < 80; x++)
                destination[x] = (source[x] << 16) | (source[x + 1] >> 16);
            break;
        case 3:
            #pragma GCC unroll 8
            for (uint16_t x = 0; x < 80; x++)
                destination[x] = (source[x] << 24) | (source[x + 1] >> 8);
            break;
        }
        if (++by == bg_height) by=0;
    }
}

void gdl_render_rows_flat(const GdlScene *,const GdlFrame *,const GdlSurface *,uint16_t,uint16_t);
/* Low-priority background bands use the existing compositor. Only bands with
 * high BG tiles require the more expensive two-plane priority ordering. */
void gdl_render_rows_planes(const GdlScene *scene,const GdlFrame *frame,
                            const GdlSurface *surface,uint16_t first,uint16_t end) {
    if (end>GDL_HEIGHT) end=GDL_HEIGHT;
    uint16_t height=scene->background.height_tiles*8;
    if (!height) return;
    GdlFrame band=*frame;
    band.background_priority=0;
    while (first<end) {
        uint16_t by=((uint32_t)first+frame->background_y)%height;
        uint16_t next=first+8-(by&7);
        if (next>end) next=end;
        bool high=false;
        uint32_t index=(uint32_t)(by>>3)*scene->background.width_tiles;
        if (scene->background.width_tiles>=8) {
            for (uint16_t i=0;i<scene->background.width_tiles/8;i++)
                if (frame->background_priority[(index>>3)+i]) { high=true; break; }
        } else {
            for (uint16_t i=0;i<scene->background.width_tiles;i++)
                if (priority_at(frame->background_priority,index+i)) { high=true; break; }
        }
        bool opaque=frame->background_opaque_rows && frame->background_opaque_rows[by];
        if (opaque) {
            for (uint16_t y=first+1;y<next;y++) {
                uint16_t row=((uint32_t)y+frame->background_y)%height;
                if (!frame->background_opaque_rows[row]) { next=y; break; }
            }
            render_opaque_rows(scene,frame,surface,first,next);
        }
        else if (high) render_priority_rows(scene,frame,surface,first,next);
        else {
            band.background_y=by-first;
            gdl_render_rows_flat(scene,&band,surface,first,next);
        }
        first=next;
    }
}
