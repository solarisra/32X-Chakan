#include <assert.h>
#include <string.h>
#include "gamestate.h"
#include "gamestates/gamestate_logo.h"
#include "level_manager.h"
#include "gamestates/gamestate_menu.h"
#include "gamestates/gamestate_intro.h"
static unsigned time_now,cues,play_init,play_updates,state_changes,letter_cues,intro_cues;
void gamestate_intro_lightning(void) { intro_cues++; }
void gamestate_menu_letter(void) {
    assert(currentMenuState==MENU_SPLASH);
    assert(gamestate_render_frame()->sprite_count==0);
    letter_cues++;
}
static uint16_t palette[80];
static GdlScene active_scene;
void gdl_shared_upload(void *dst,const void *src,uint32_t bytes) { memcpy(dst,src,bytes); }
void gdl_palette_load(uint16_t first,const uint16_t *colors,uint16_t count) {
    assert(first+count<=80);
    for (unsigned i=0;i<count;i++) palette[first+i]=colors[i];
}
void gdl_palette_schedule(uint16_t first,const uint16_t *colors,uint16_t count) {
    gdl_palette_load(first,colors,count);
}
bool gdl_scheduler_init(const GdlScene *scene) {
    assert(gdl_scene_valid(scene)); active_scene=*scene; return true;
}
static void check_lightning_priority(void) {
    static uint32_t pixels[224*80];
    const GdlFrame *frame=gamestate_render_frame();
    assert(frame->background_priority && !frame->sprite_count);
    GdlSurface surface={pixels,80};
    gdl_render_rows(&active_scene,frame,&surface,0,224);
    unsigned portrait_pixels=0,lightning_pixels=0;
    for(unsigned y=0;y<224;y++) for(unsigned x=0;x<320;x++) {
        const GdlTilemap *b=&active_scene.background,*f=&active_scene.foreground;
        unsigned shift=(3-(x&3))*8, word=(x&7)/4;
        uint32_t bg=b->tiles[b->map[(y/8)*b->width_tiles+x/8]+(y&7)*2+word];
        uint32_t fg=f->tiles[f->map[(y/8)*f->width_tiles+x/8]+(y&7)*5+word];
        unsigned bp=(bg>>shift)&255,fp=(fg>>shift)&255;
        if(bp&15) portrait_pixels++;
        if(fp) lightning_pixels++;
        unsigned expected=(bp&15)?bp:fp?fp:bp;
        assert(((pixels[y*80+x/4]>>shift)&255)==expected);
    }
    assert(portrait_pixels>0 && lightning_pixels>0);
}
static void check_intro_pixels(void) {
    static uint32_t expected[224*80],actual[224*80];
    GdlFrame reference=*gamestate_render_frame();
    assert(reference.screen);
    GdlSurface a={expected,80},b={actual,80};
    reference.screen=0;
    gdl_render_rows(&active_scene,&reference,&a,0,224);
    gdl_render_rows(&active_scene,gamestate_render_frame(),&b,0,112);
    gdl_render_rows(&active_scene,gamestate_render_frame(),&b,112,224);
    assert(memcmp(expected,actual,sizeof(expected))==0);
}
void gamestate_logo_scream(void) { assert(time_now==189); cues++; }
void gamestate_changed(GAME_STATE state) {
    assert(state==STATE_LOGO || state==STATE_INTRO || state==STATE_MENU || state==STATE_PLAY); state_changes++;
}
void level_manager_init(void) { assert(currentMenuState==MAIN_MENU); play_init++; }
void level_manager_update(uint16_t pad) { assert(pad==0x80); play_updates++; }
const GdlFrame *level_manager_render_frame(void) { return 0; }
int main(void) {
    gamestate_init();
    assert(currentState==STATE_LOGO);
    const GdlFrame *frame=gamestate_render_frame();
    assert(frame->sprite_count==1 && frame->sprites[0].x==114 && frame->sprites[0].y==80);
    assert(frame->sprites[0].image->width==96 && frame->sprites[0].image->height==32);
    for (time_now=1;time_now<=373;time_now++) {
        gamestate_update(0x80); /* Source logo does not skip on Start. */
        if (time_now<373) assert(currentState==STATE_LOGO);
        if (time_now==3) {
            /* Slots 10..13 reach the dark endpoint, never wrap to cyan. */
            for(unsigned i=10;i<=13;i++) assert(palette[i]==(13u<<10));
            assert(palette[9]==(17u<<10));
        }
        if (time_now>=348 && time_now<373) for (int i=0;i<16;i++) assert(palette[i]==0);
    }
    assert(currentState==STATE_INTRO && cues==1 && play_init==0 && state_changes==2);
    assert(gamestate_render_frame()->sprites[0].x==127);
    gamestate_update(0x80); /* Held logo Start does not skip the new state. */
    assert(currentState==STATE_INTRO && intro_cues==1);
    unsigned panels=0,last_height=0;
    for(unsigned i=1;i<5811;i++) {
        assert(currentState==STATE_INTRO);
        const GdlFrame *f=gamestate_render_frame();
        assert(gdl_frame_valid(&active_scene,f));
        if(active_scene.foreground.height_tiles!=last_height) {
            last_height=active_scene.foreground.height_tiles; panels++;
        }
        if(i%100==0 || (i>=220 && i<270)) check_intro_pixels();
        if(i==225) assert(f->camera_y>0 && f->sprites[0].frame<6);
        gamestate_update(0);
        if(i%2==0) gamestate_frame_presented();
    }
    assert(panels==3); /* 880px crawl, three 224px panels, 816px crawl. */
    assert(currentState==STATE_MENU && intro_cues==1 && state_changes==3);
    gamestate_update(0); assert(currentMenuState==MENU_SPLASH);
    for(unsigned i=0;i<420;i++) {
        gamestate_update(0);
        if(i==1) {
            assert(!gamestate_render_frame()->background_priority);
            assert(palette[5]==(4u<<10));
            assert(palette[6]==(8u<<10));
            assert(palette[7]==((4u<<5)|(8u<<10)));
            assert(palette[8]==(8u|(17u<<5)|(22u<<10)));
        }
        if(i==130) check_lightning_priority();
        if(i<294) assert(letter_cues==0);
        if(i>=294 && i<372) assert(letter_cues==1+(i-294)/13);
    }
    assert(!gamestate_render_frame()->background_priority);
    assert(currentMenuState==SPLASH_START && letter_cues==6);
    gamestate_update(0x80); assert(currentMenuState==MAIN_MENU);
    gamestate_update(0x80); assert(currentState==STATE_MENU);
    gamestate_update(8); gamestate_update(0x80); assert(currentMenuState==OPTIONS_MENU);
    gamestate_update(0); gamestate_update(0x80); assert(currentMenuState==MAIN_MENU);
    gamestate_update(0); gamestate_update(0x80);
    assert(currentState==STATE_PLAY && play_init==1 && state_changes==4 && letter_cues==6);
    gamestate_update(0x80); assert(play_updates==1 && !gamestate_render_frame());
    /* A one-tick pulse survives its off tick when rendering every two ticks. */
    gamestate_intro_init(0);
    assert(!gamestate_intro_update(0)); /* tick 0 */
    gamestate_intro_presented();
    assert(!gamestate_intro_update(0)); /* tick 1: on */
    assert(!gamestate_intro_update(0)); /* tick 2: off in the source */
    assert(palette[43]!=0 && palette[46]!=0);
    gamestate_intro_presented();
    assert(!gamestate_intro_update(0)); /* tick 3: second pulse */
    assert(!gamestate_intro_update(0)); /* tick 4: off */
    assert(palette[43]!=0);
    gamestate_intro_presented();
    assert(!gamestate_intro_update(0));
    assert(!gamestate_intro_update(0));
    assert(palette[43]==0 && palette[46]==0);
    /* Skip fades for ten ticks and consumes Start before handing off to Menu. */
    gamestate_intro_init(0);
    for(unsigned i=0;i<240;i++) assert(!gamestate_intro_update(0));
    for(unsigned i=0;i<9;i++) assert(!gamestate_intro_update(0x80));
    assert(gamestate_intro_update(0x80));
    for(unsigned i=0;i<64;i++) assert(palette[i]==0);
    return 0;
}
