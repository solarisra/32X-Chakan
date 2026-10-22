#include "gamestates/gamestate_menu.h"
#include "input_controls.h"
#include "gdl/level_assets.h"
#include "menu_data.h"
/* Explicit dimensions keep the generated names lowercase. */
static const GdlTilemap planes[]={
 {menu_far_tiles,menu_far_map,MENU_FAR_W,MENU_FAR_H,0},
 {menu_close_tiles,menu_close_map,MENU_CLOSE_W,MENU_CLOSE_H,0},
 {menu_splash_tiles,menu_splash_map,MENU_SPLASH_W,MENU_SPLASH_H,0},
 {menu_main_tiles,menu_main_map,MENU_MAIN_W,MENU_MAIN_H,0},
 {menu_options_tiles,menu_options_map,MENU_OPTIONS_W,MENU_OPTIONS_H,0}};
static const GdlSpanImage *const letters[]={&menu_C_image,&menu_h_image,&menu_a_image,&menu_k_image,&menu_a_image,&menu_n_image};
static const int16_t positions[]={7,47,79,111,143,175};
static GdlScene scene;
static GDL_LEVEL_ASSETS(menu) GdlFrame frame;
static uint16_t tick,previous;
static uint8_t final_flash;
/* Final double flash: advanced 60 ticks to match the preserved recording. */
#define FINAL_FLASH_TICK 320
static GDL_LEVEL_ASSETS(menu) uint16_t colors[48];
MenuState currentMenuState;
uint8_t menuSelection;
static const uint8_t portrait[16][3]={{0,0,0},{0,0,0},{0,1,0},{0,0,1},{0,1,1},{1,1,1},{0,2,0},{0,1,3},{1,2,2},{0,2,3},{1,2,3},{2,3,3},{2,3,4},{3,0,0},{5,0,0},{7,2,0}};
static const uint8_t shadow[16][3]={{0,0,0},{0,0,0},{0,0,0},{0,0,0},{0,0,0},{0,0,0},{0,0,1},{0,0,1},{0,0,0},{0,0,1},{0,0,0},{0,1,1},{0,1,2},{0,0,0},{2,0,0},{4,0,0}};
static uint16_t rgb(unsigned r,unsigned g,unsigned b) {return r*31/7 | ((g*31/7)<<5) | ((b*31/7)<<10);}
static void plane(unsigned index) {
 scene.foreground=planes[index];
 /* Original BG_B is high priority; lightning on BG_A is low priority.
  * Text screens use high BG_A, so their ordinary foreground order suffices. */
 frame.background_priority=index==1?menu_back_priority:0;
 gdl_scheduler_init(&scene);
}
static void screen(MenuState state) {
 currentMenuState=state; menuSelection=0; final_flash=0;
 plane(state==SPLASH_START?2:state==MAIN_MENU?3:4);
}
void gamestate_menu_init(uint16_t pad) {
 frame=(GdlFrame){0}; final_flash=0; tick=0; previous=pad; menuSelection=0; currentMenuState=MENU_SPLASH;
 for(unsigned i=0;i<48;i++) colors[i]=0;
 scene.background=(GdlTilemap){menu_back_tiles,menu_back_map,MENU_BACK_W,MENU_BACK_H,0};
 plane(0); gdl_palette_load(0,colors,48);
}
bool gamestate_menu_update(uint16_t pad) {
 uint16_t pressed=pad & ~previous; previous=pad;
 if(pressed & BUTTON_START) {
  if(currentMenuState<=SPLASH_START) screen(MAIN_MENU);
  else if(currentMenuState==MAIN_MENU) {if(!menuSelection) return true; screen(OPTIONS_MENU);}
  else if(!menuSelection) screen(MAIN_MENU);
 }
 if(currentMenuState>=MAIN_MENU) {
  if(pad & BUTTON_LEFT) menuSelection=0;
  else if(pad & BUTTON_RIGHT) menuSelection=1;
 }
 if(currentMenuState==MENU_SPLASH) {
  if(tick==100) plane(1);
  for(unsigned i=1;i<=8;i++) colors[i]=0;
  unsigned group=0;
  if(tick==FINAL_FLASH_TICK) final_flash=1;
  bool final_lit=final_flash==1 || final_flash==2 || final_flash==4 || final_flash==5;
  if(tick<100) {if(tick==1 || tick==2 || tick==4 || tick==5) group=5;
   if(tick==50 || tick==51 || tick==53 || tick==54) group=1;
  } else if((tick>=130 && tick<133)||(tick>=134 && tick<137)||(tick>=199 && tick<201)||(tick>=202 && tick<204)||final_lit) group=1;
  if(group) {
   colors[group]=rgb(0,0,1);
   colors[group+1]=tick<100?rgb(0,0,2):rgb(0,0,3);
   colors[group+2]=tick<100?rgb(0,1,2):rgb(0,2,5);
   colors[group+3]=tick<100?rgb(2,4,5):rgb(1,4,6);
  }
  if(tick>=130 && tick<140) for(unsigned i=0;i<16;i++) colors[16+i]=group?rgb(shadow[i][0],shadow[i][1],shadow[i][2]):0;
  if(tick>=150) for(unsigned i=0;i<16;i++) {
   unsigned scale=tick<160?tick-150:10; uint16_t c=rgb(portrait[i][0],portrait[i][1],portrait[i][2]);
   colors[16+i]=((c&31)*scale/10)|((((c>>5)&31)*scale/10)<<5)|((((c>>10)&31)*scale/10)<<10);
  }
  if(tick>=410) screen(SPLASH_START);
  tick++;
 }
 if(currentMenuState!=MENU_SPLASH) {colors[15]=rgb(0,6,6); for(unsigned i=0;i<16;i++) colors[16+i]=rgb(portrait[i][0],portrait[i][1],portrait[i][2]);}
 frame.sprite_count=0;
 unsigned count=currentMenuState==MENU_SPLASH?(tick<286?0:1+(tick-286)/13):6;
 if(count>6) count=6;
 /* Embedded title impacts start around 4.76 seconds in the unchanged WAV.
  * Advance the visuals instead of editing or interrupting the recording. */
 for(unsigned i=0;i<count;i++) frame.sprites[frame.sprite_count++]=(GdlSprite){letters[i],positions[i],7,0,false,0};
 for(unsigned i=0;i<16;i++) {
  uint16_t c=menu_a_palette[i];
  if(currentMenuState==MENU_SPLASH && tick<374) c=rgb(2,0,0);
  else if(currentMenuState==MENU_SPLASH && tick<379) {
   unsigned n=tick-374;
   c=(((c&31)*n+8*(5-n))/5)|((((c>>5)&31)*n/5)<<5)|((((c>>10)&31)*n/5)<<10);
  }
  colors[32+i]=c;
 }
 if(currentMenuState>=MAIN_MENU) frame.sprites[frame.sprite_count++]=(GdlSprite){&menu_arrow_image,60+120*menuSelection,176,0,false,0};
 if(tick>=160 || currentMenuState!=MENU_SPLASH) {
  static const uint8_t eyes[4][3]={{2,4,6},{1,3,5},{2,4,6},{3,5,7}};
  const uint8_t *e=eyes[(tick/16)&3]; for(unsigned i=0;i<3;i++) colors[29+i]=rgb(e[i],i==2?1:0,0);
 }
 if(currentMenuState!=MENU_SPLASH) tick++;
 gdl_palette_schedule(0,colors,48); return false;
}
const GdlFrame *gamestate_menu_frame(void) {return &frame;}

void gamestate_menu_presented(void) {
 /* Advance on completed presentations, not catch-up simulation updates.
  * Both bright pulses and their dark separation must actually reach screen. */
 if(currentMenuState==MENU_SPLASH && final_flash>=1 && final_flash<=6) final_flash++;
}
