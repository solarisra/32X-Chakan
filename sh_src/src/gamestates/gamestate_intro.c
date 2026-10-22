#include "gamestates/gamestate_intro.h"
#include "input_controls.h"
#include "gdl/level_assets.h"
#include "intro_data.h"
static GDL_LEVEL_ASSETS(intro) GdlFrame frame;
static GDL_LEVEL_ASSETS(intro) GdlScene scene;
static uint16_t tick,previous;
static GDL_LEVEL_ASSETS(intro) uint16_t colors[64],skip_colors[64];
static uint8_t panel,skip_tick,pending_flashes;
static GDL_LEVEL_ASSETS(intro) uint32_t screen_pixels[224*80];
static GDL_LEVEL_ASSETS(intro) uint32_t text_patches[INTRO_PATCH_WORDS];
static GDL_LEVEL_ASSETS(intro) uint16_t text_rows[881];
static GDL_LEVEL_ASSETS(intro) uint32_t animation_pixels[6*88*30];
static GDL_LEVEL_ASSETS(intro) GdlScreen screen;
static const uint32_t *const screens[]={intro_1_screen,intro_2_screen,intro_3_screen,intro_4_screen,intro_5_screen};
static const uint32_t *const patches[]={intro_1_patches,intro_2_patches,intro_3_patches,intro_4_patches,intro_5_patches};
static const uint16_t *const patch_rows[]={intro_1_patch_rows,intro_2_patch_rows,intro_3_patch_rows,intro_4_patch_rows,intro_5_patch_rows};
static const uint32_t patch_sizes[]={sizeof(intro_1_patches),sizeof(intro_2_patches),sizeof(intro_3_patches),sizeof(intro_4_patches),sizeof(intro_5_patches)};
/* Blocking fades/cleanup from the source are represented as frame ticks. */
static const uint16_t duration[]={2343,522,522,522,1902};
static const GdlTilemap *const foreground[]={&intro_1_fg,&intro_2_fg,&intro_3_fg,&intro_4_fg,&intro_5_fg};
static const GdlTilemap *const background[]={&intro_1_bg,&intro_2_bg,&intro_3_bg,&intro_4_bg,&intro_5_bg};
static const uint16_t *const palettes[]={intro_1_fg_palette,intro_2_fg_palette,intro_3_fg_palette,intro_4_fg_palette,intro_5_bg_palette};
static uint16_t scale(uint16_t c,unsigned amount,unsigned total) {
 return ((c&31)*amount/total)|((((c>>5)&31)*amount/total)<<5)|((((c>>10)&31)*amount/total)<<10);
}
static uint16_t rgb(unsigned r,unsigned g,unsigned b) {
 return r*31/7|((g*31/7)<<5)|((b*31/7)<<10);
}
static bool flash(unsigned t,unsigned start,unsigned repeat) {
 if(t<start) return false;
 unsigned phase=(t-start)%repeat;
 return phase==0 || phase==2;
}
static void load_panel(void) {
 tick=0; pending_flashes=0; frame=(GdlFrame){0};
 scene=(GdlScene){*foreground[panel],*background[panel]};
 gdl_shared_upload(screen_pixels,screens[panel],sizeof(screen_pixels));
 gdl_shared_upload(text_patches,patches[panel],patch_sizes[panel]);
 gdl_shared_upload(text_rows,patch_rows[panel],(scene.foreground.height_tiles*8+1)*sizeof(uint16_t));
 if(panel==0) gdl_shared_upload(animation_pixels,intro_animation,sizeof(animation_pixels));
 GdlScreen prepared={screen_pixels,text_patches,panel==0?animation_pixels:0,text_rows,31,55,30,88};
 gdl_shared_upload(&screen,&prepared,sizeof(screen));
 frame.screen=&screen;
 if(panel==0) {
  frame.sprite_count=1;
  frame.sprites[0]=(GdlSprite){&intro_chakan,127,55,0,false,GDL_SPRITE_BEHIND_FOREGROUND};
 }
 for(unsigned i=0;i<64;i++) colors[i]=0;
 gdl_palette_schedule(0,colors,64);
 gdl_scheduler_init(&scene);
}
void gamestate_intro_init(uint16_t pad) {
 panel=0; skip_tick=0; previous=pad; load_panel();
}
bool gamestate_intro_update(uint16_t pad) {
 uint16_t pressed=pad & ~previous; previous=pad;
 if(!skip_tick && (pressed & BUTTON_START)) {
  for(unsigned i=0;i<64;i++) skip_colors[i]=colors[i];
  skip_tick=1;
 }
 if(skip_tick) {
  unsigned remaining=skip_tick<5?5-skip_tick:0;
  for(unsigned i=0;i<64;i++) colors[i]=scale(skip_colors[i],remaining,5);
  gdl_palette_schedule(0,colors,64);
  return skip_tick++>=10;
 }
 if(panel==0 && tick==0) gamestate_intro_lightning();
 for(unsigned i=0;i<64;i++) colors[i]=0;
 unsigned fade=20;
 if(panel==0) {
  fade=tick<200?0:tick<220?tick-199:20;
  if(tick>=2300) fade=tick<2320?2319-tick:0;
 } else {
  if(tick<20) fade=tick+1;
  unsigned end=panel==4?1881:480;
  if(tick>=end) fade=tick<end+20?end+19-tick:0;
 }
 for(unsigned i=0;i<16;i++) colors[i]=scale(palettes[panel][i],fade,20);
 colors[0]=0; /* Genesis palette-zero pixels use the black backdrop. */
 if(panel==0) {
  /* The source overrides PAL0 slot 7 with its external black palette entry. */
  colors[7]=0;
  for(unsigned i=1;i<16;i++) colors[16+i]=scale(intro_chakan_palette[i],fade,20);
  unsigned logical=tick<220?(tick<200?tick:200):tick-20;
  bool flashes[4]={logical==50 || logical==52 || logical==150 || logical==152 || flash(logical,300,200),
                   flash(logical,350,200),flash(logical,250,200),
                   logical==1 || logical==3 || logical==100 || logical==102 || flash(logical,400,200)};
  static const uint8_t starts[]={33,37,40,43},counts[]={4,3,3,4};
  /* Catch-up may consume both on/off ticks before either SH-2 renders.
   * Retain each pulse until a completed frame actually presents it. */
  for(unsigned group=0;group<4;group++) if(flashes[group]) pending_flashes|=1u<<group;
  for(unsigned group=0;group<4;group++) if(pending_flashes & (1u<<group)) {
   static const uint8_t shades[4][3]={{0,0,1},{0,0,2},{0,1,2},{2,4,5}};
   for(unsigned i=0;i<counts[group];i++) {
    uint16_t c=rgb(shades[i][0],shades[i][1],shades[i][2]);
    if(group==0 && logical<200 && i==1) c=rgb(0,0,1);
    colors[starts[group]+i]=tick>=2300?scale(c,fade,20):c;
   }
  }
  /* SGDK's f32 step is trunc(0.005*1024)=5. Its double conversion
   * followed by >>4 in the source gives a 5/16 pixel crawl per update. */
  unsigned steps=tick>=221?tick-220:0;
  if(steps>2079) steps=2079;
  frame.camera_y=steps*5/16;
  unsigned animated=tick<200?tick:tick<220?200:tick<2300?tick-20:2280;
  frame.sprites[0].frame=(animated/8)%6;
 } else if(panel==4) {
  for(unsigned i=1;i<16;i++) colors[16+i]=scale(intro_5_fg_palette[i],fade,20);
  unsigned phase=((tick+1)/11)%3;
  for(unsigned i=0;i<3;i++) colors[13+i]=scale(intro_5_bg_palette[13+(i+phase)%3],fade,20);
  unsigned y=tick?((unsigned)tick*5/16):0;
  frame.camera_y=y>592?592:y;
 }
 gdl_palette_schedule(0,colors,64);
 if(++tick>=duration[panel]) {
  if(++panel==5) return true;
  load_panel();
 }
 return false;
}
const GdlFrame *gamestate_intro_frame(void) { return &frame; }

void gamestate_intro_presented(void) { pending_flashes=0; }
