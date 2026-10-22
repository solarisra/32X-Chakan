#include <assert.h>
#include <string.h>
#include "gamestate.h"
#include "gamestates/gamestate_logo.h"
#include "level_manager.h"
#include "gamestates/gamestate_menu.h"
#include "gamestates/gamestate_intro.h"
#include "pause_menu.h"
#include "input_controls.h"
static unsigned time_now,cues,play_init,play_updates,state_changes,intro_cues;
void gamestate_intro_lightning(void) { intro_cues++; }
static uint16_t palette[96],synced_pad;
static unsigned opens,closes;
void gamestate_play_menu_changed(bool open) {if(open) opens++;else closes++;}
void level_manager_sync_input(uint16_t pad) {synced_pad=pad;}
static GdlScene active_scene;
void gdl_shared_upload(void *dst,const void *src,uint32_t bytes) { memcpy(dst,src,bytes); }
void gdl_palette_load(uint16_t first,const uint16_t *colors,uint16_t count) {
    assert(first+count<=96);
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
void level_manager_update(uint16_t pad) { (void)pad; play_updates++; }
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
        if(i>=318 && i<=324) {
            bool lit=i==319 || i==320 || i==322 || i==323;
            assert((palette[1]!=0)==lit);
        }
        if(i>=379 && i<=384) assert(palette[1]==0);
        if(i<284) assert(gamestate_render_frame()->sprite_count==0);
        if(i>=284 && i<362) assert(gamestate_render_frame()->sprite_count==1+(i-284)/13);
        gamestate_frame_presented();
    }
    assert(!gamestate_render_frame()->background_priority);
    assert(currentMenuState==SPLASH_START);
    gamestate_update(0x80); assert(currentMenuState==MAIN_MENU);
    gamestate_update(0x80); assert(currentState==STATE_MENU);
    gamestate_update(8); gamestate_update(0x80); assert(currentMenuState==OPTIONS_MENU);
    gamestate_update(0); gamestate_update(0x80); assert(currentMenuState==MAIN_MENU);
    gamestate_update(0); gamestate_update(0x80);
    assert(currentState==STATE_PLAY && play_init==1 && state_changes==4);
    gamestate_update(0x80); assert(play_updates==1 && !gamestate_render_frame());
    gamestate_update(0); unsigned frozen=play_updates;
    gamestate_update(BUTTON_START);
    assert(gamestate_is_paused() && currentState==STATE_PLAY && opens==1);
    for(unsigned i=0;i<8;i++) gamestate_update(BUTTON_START);
    assert(play_updates==frozen && gamestate_is_paused());
    /* Direction/action buttons have no menu function in this first slice. */
    gamestate_update(0);gamestate_update(BUTTON_RIGHT|BUTTON_A|BUTTON_C);
    assert(gamestate_is_paused() && play_updates==frozen);
    GdlFrame paused_frame;assert(gamestate_prepare_frame(&paused_frame)==&paused_frame);
    assert(paused_frame.screen && !paused_frame.screen->animation && paused_frame.sprite_count==0);
    assert(gdl_frame_valid(&active_scene,&paused_frame));
    gamestate_update(0);gamestate_update(BUTTON_START);
    assert(!gamestate_is_paused() && closes==1 && synced_pad==BUTTON_START && play_updates==frozen);
    gamestate_update(BUTTON_START); /* Held resume does not immediately reopen. */
    assert(!gamestate_is_paused() && play_updates==frozen+1);
    gamestate_update(BUTTON_RETURN_HUB);
    assert(!gamestate_is_paused() && play_updates==frozen+2);
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
    /* Catch-up crosses the entire old flash window before presentation. */
    gamestate_menu_init(0);
    for(unsigned i=0;i<=324;i++) assert(!gamestate_menu_update(0));
    assert(palette[1]!=0);
    const bool phases[]={true,false,true,true,false,false};
    for(unsigned i=0;i<6;i++) {
        gamestate_menu_presented();
        for(unsigned step=0;step<4;step++) assert(!gamestate_menu_update(0));
        assert((palette[1]!=0)==phases[i]);
    }
    return 0;
}
