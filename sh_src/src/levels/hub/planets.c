#include "planets.h"
#include "portal_hub_data.h"
#define PLANET(name,w,h) {hub_##name##_rows,hub_##name##_spans,hub_##name##_pixels,w,h,1}
static const GdlSpanImage images[]={PLANET(green_planet,32,32),PLANET(small_planets,24,24),
    PLANET(planet_moons,48,48),PLANET(ringed_planet,96,40)};
/* Two instances per type across the 1920px Hub, with 212px clear gaps between neighboring sprite bounds.
 * Each horizontal position has one vertically wrapping planet. */
static const int16_t positions[][2]={{16,10},{260,330},{496,650},{756,970}};
static const uint16_t speeds[]={192,256,320,384}; /* 8.8 px/tick; stars use 128. */
void hub_planets_submit(GdlFrame *frame,uint32_t tick) {
    for (uint16_t i=0;i<4;i++) {
        const GdlSpanImage *image=&images[i];
        /* Include the image height so it fully exits the level before wrapping.
         * Use the full tick count: fractional motion survives skipped renders. */
        uint16_t period=1280+image->height;
        uint32_t phase=((uint64_t)tick*speeds[i]>>8)%period;
        int16_t y=(positions[i][1]+image->height-(int32_t)phase)%period;
        if (y<0) y+=period;
        y-=image->height;
        y-=frame->camera_y;
        if (y+image->height<=0 || y>=GDL_HEIGHT) continue;
        for (uint16_t copy=0;copy<2;copy++) {
            int16_t x=positions[i][0]+copy*1048-frame->camera_x;
            if (x+image->width<=0 || x>=GDL_WIDTH) continue;
            if (frame->sprite_count>=GDL_MAX_SPRITES-1) return;
            frame->sprites[frame->sprite_count++]=(GdlSprite){image,x,y,0,false,GDL_SPRITE_BEHIND_FOREGROUND};
        }
    }
}
