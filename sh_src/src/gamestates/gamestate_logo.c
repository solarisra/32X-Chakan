#include "gamestates/gamestate_logo.h"
#include "gdl/level_assets.h"
#include "logo_data.h"
static const GdlSpanImage image={logo_rows,logo_spans,logo_pixels,96,32,1};
static const GdlScene scene={
    {logo_empty_tiles,logo_empty_map,40,28,logo_empty_rows},
    {logo_black_tiles,logo_black_map,1,1,0}
};
static GDL_LEVEL_ASSETS(logo) GdlFrame frame;
static uint16_t tick;
static GDL_LEVEL_ASSETS(logo) uint16_t colors[16];
static uint8_t indices[11],delay;
static int8_t directions[11];
static const uint8_t blue_cycle[11][3]={
    {0,192,255},{0,160,255},{0,128,255},{0,96,255},{0,64,255},{0,32,255},
    {0,0,224},{0,0,192},{0,0,160},{0,0,128},{0,0,96}
};
static const uint8_t red_cycle[11][3]={
    {255,192,0},{255,160,0},{255,128,0},{255,96,0},{255,64,0},{255,32,0},
    {224,0,0},{192,0,0},{160,0,0},{128,0,0},{96,0,0}
};
static void palette(const uint16_t *source) {
    for (uint16_t i=0;i<16;i++) colors[i]=source[i];
    colors[0]=0;
    gdl_palette_schedule(0,colors,16);
}
static void cycle(const uint8_t values[11][3]) {
    if (delay++<2) return;
    delay=0;
    for (uint16_t i=0;i<11;i++) {
        const uint8_t *rgb=values[indices[i]];
        /* Quantize through Genesis 3-bit RGB to retain the source palette. */
        colors[i+3]=((rgb[0]>>5)*31/7) | (((rgb[1]>>5)*31/7)<<5) | (((rgb[2]>>5)*31/7)<<10);
        int16_t next=indices[i]+directions[i];
        if (next>=10) { next=10; directions[i]=-1; }
        else if (next<=0) { next=0; directions[i]=1; }
        indices[i]=next;
    }
    gdl_palette_schedule(0,colors,16);
}
void gamestate_logo_init(void) {
    frame=(GdlFrame){0};
    frame.sprite_count=1;
    /* Visible artwork occupies columns 0..91; center those 92 pixels. */
    frame.sprites[0]=(GdlSprite){&image,(GDL_WIDTH-92)/2,80,0,false,GDL_SPRITE_ABOVE_FOREGROUND};
    static const uint16_t black[80]={0};
    gdl_palette_load(0,black,80);
    gdl_scheduler_init(&scene);
    palette(logo_blue);
    /* Saturate the staggered phases at the dark endpoint. Wrapping 11..13
     * to 0..2 makes the upper-right gradient pixels jump to bright cyan.
     * The source clamps at this endpoint when advancing its oscillator. */
    for (uint16_t i=0;i<11;i++) {
        indices[i]=i+3<11?i+3:10;
        directions[i]=1;
    }
    tick=0; delay=0;
}
bool gamestate_logo_update(void) {
    if (tick<123) cycle(blue_cycle);
    else if (tick>=183 && tick<283) {
        if (tick==183) palette(logo_red);
        if (tick==188) gamestate_logo_scream();
        cycle(red_cycle);
    } else if (tick>=343 && tick<348) {
        uint16_t faded[16],remaining=347-tick;
        for (uint16_t i=0;i<16;i++) {
            uint16_t c=colors[i];
            faded[i]=((c&31)*remaining/5)|((((c>>5)&31)*remaining/5)<<5)
                |((((c>>10)&31)*remaining/5)<<10);
        }
        gdl_palette_schedule(0,faded,16);
    }
    /* Source repeats the blocking fade five times: retain its final black hold. */
    return ++tick>=373;
}
const GdlFrame *gamestate_logo_frame(void) { return &frame; }
