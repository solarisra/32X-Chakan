#include "gdl/level_assets.h"
#include "hub_sprites.h"
#include "planets.h"
#include "level_progress.h"
#include "player_sprite.h"
#include "portal_hub_data.h"
static GDL_LEVEL_ASSETS(hub) uint16_t character_rows[sizeof(hub_chakan_rows) / 2], character_spans[sizeof(hub_chakan_spans) / 2];
static GDL_LEVEL_ASSETS(hub) uint8_t character_pixels[sizeof(hub_chakan_pixels)];
static GDL_LEVEL_ASSETS(hub) uint16_t flame_rows[sizeof(hub_flame_rows) / 2], flame_spans[sizeof(hub_flame_spans) / 2];
static GDL_LEVEL_ASSETS(hub) uint8_t flame_pixels[sizeof(hub_flame_pixels)];
static const GdlSpanImage actions = {hub_actions_rows, hub_actions_spans, hub_actions_pixels, 112, 80, 5};
static const GdlSpanImage chakan = {character_rows, character_spans, character_pixels, 112, 80, 12};
static const GdlSpanImage flame = {flame_rows, flame_spans, flame_pixels, 16, 32, 6};
/* Small scenery remains immutable in ROM so it does not enlarge the RAM overlay. */
#define ITEM(name,w,h,n) {hub_item_##name##_rows,hub_item_##name##_spans,hub_item_##name##_pixels,w,h,n}
static const GdlSpanImage potions[]={ITEM(earth,16,16,4),ITEM(fire,16,16,4),
    ITEM(water,16,16,4),ITEM(air,16,24,4)};
static const GdlSpanImage hourglass=ITEM(hourglass,24,32,1);
static const int16_t potion_positions[][2]={{685,665},{904,665},{685,702},{904,694}};
static const int16_t hourglass_positions[][2]={{679,606},{898,606},{679,739},{898,739}};
/* Combine all 24 completion indicators into one sprite so scenery and the
 * player cannot exhaust the 16-sprite frame budget when a realm is complete. */
static const int16_t stage_positions[16][2]={{786,674},{780,668},{763,652},{757,646},
 {807,674},{813,668},{830,652},{836,646},{786,695},{780,701},{763,717},{757,723},
 {807,695},{813,701},{830,717},{836,723}};
static const int16_t level_positions[8][2]={{769,658},{745,633},{820,658},{844,633},
 {769,707},{745,732},{820,707},{844,732}};
static GDL_LEVEL_ASSETS(hub) uint16_t marker_rows[231],marker_spans[512*3];
static GdlSpanImage markers;
static uint32_t marker_bits;
static bool markers_ready,markers_visible;
static void update_markers(void) {
 uint32_t bits=level_progress_bits();
 if(markers_ready && marker_bits==bits) return;
 marker_bits=bits; markers_ready=true;
 unsigned count=0;
 for(unsigned y=0;y<230;y++) {
  marker_rows[y]=count;
  for(unsigned i=0;i<24;i++) {
   bool stage=i<16;
   unsigned index=stage?i:i-16;
   bool visible=stage?level_progress_stage(index%4/2,index/4,index%2):
       level_progress_group(index%2,index/2);
   if(!visible) continue;
   const int16_t *position=stage?stage_positions[index]:level_positions[index];
   unsigned size=stage?8:16;
   int local_y=(int)(y%115)+633-position[1];
   if(local_y<0 || local_y>=(int)size) continue;
   const uint16_t *rows=stage?hub_marker_stage_rows:hub_marker_level_rows;
   const uint16_t *spans=stage?hub_marker_stage_spans:hub_marker_level_spans;
   unsigned row=local_y+(y>=115?size:0);
   int x=position[0]-745;
   if(y>=115) x=115-x-size;
   for(unsigned run=rows[row];run<rows[row+1];run++) {
    if(count>=512) continue;
    marker_spans[count*3]=x+spans[run*3];
    marker_spans[count*3+1]=spans[run*3+1];
    marker_spans[count*3+2]=spans[run*3+2];
    count++;
   }
  }
 }
 marker_rows[230]=count; markers_visible=count!=0;
 gdl_shared_upload(marker_rows,marker_rows,sizeof(marker_rows));
 gdl_shared_upload(marker_spans,marker_spans,count*3*sizeof(uint16_t));
 const uint16_t *rows=marker_rows,*spans=marker_spans;
#ifdef __sh__
 /* Flags may change while Hub is bound. Read mutable row directories through
  * the uncached alias on both processors, without rebinding the whole scene. */
 rows=(const uint16_t *)((uintptr_t)rows|0x20000000UL);
 spans=(const uint16_t *)((uintptr_t)spans|0x20000000UL);
#endif
 markers=(GdlSpanImage){rows,spans,hub_marker_pixels,115,115,1};
}
#define PORTAL_STORAGE(name) \
    static GDL_LEVEL_ASSETS(hub) uint16_t name##_rows[sizeof(hub_##name##_rows) / 2]; \
    static GDL_LEVEL_ASSETS(hub) uint16_t name##_spans[sizeof(hub_##name##_spans) / 2]; \
    static GDL_LEVEL_ASSETS(hub) uint8_t name##_pixels[sizeof(hub_##name##_pixels)];
PORTAL_STORAGE(earth)
PORTAL_STORAGE(fire)
PORTAL_STORAGE(water)
PORTAL_STORAGE(air)
#define PORTAL(name) {name##_rows, name##_spans, name##_pixels, 64, 88, 1}
static const GdlSpanImage portals[] = {PORTAL(earth), PORTAL(fire), PORTAL(water), PORTAL(air)};
static const int16_t portal_positions[][2] = {{384,175}, {1696,335}, {576,1135}, {1375,1103}};
int16_t hub_portal_overlap(int16_t x,int16_t y,uint16_t width,uint16_t height) {
    for (uint16_t i=0;i<4;i++) {
        int16_t left=portal_positions[i][0], top=portal_positions[i][1];
        if (level_progress_next(i)==LEVEL_PORTAL_HUB) continue;
        if (x+width>left && x<left+64 && y+height>top && y<top+88) return i;
    }
    return -1;
}
static const int16_t flame_positions[][2] = {{135,146}, {839,338}, {103,562}, {1127,562}, {103,914}, {1351,882}, {1575,882}, {807,1042}, {999,1042}};
void hub_sprites_init(void) {
    gdl_shared_upload(character_rows, hub_chakan_rows, sizeof(character_rows));
    gdl_shared_upload(character_spans, hub_chakan_spans, sizeof(character_spans));
    gdl_shared_upload(character_pixels, hub_chakan_pixels, sizeof(character_pixels));
    gdl_shared_upload(flame_rows, hub_flame_rows, sizeof(flame_rows));
    gdl_shared_upload(flame_spans, hub_flame_spans, sizeof(flame_spans));
    gdl_shared_upload(flame_pixels, hub_flame_pixels, sizeof(flame_pixels));
#define UPLOAD_PORTAL(name) \
    gdl_shared_upload(name##_rows, hub_##name##_rows, sizeof(name##_rows)); \
    gdl_shared_upload(name##_spans, hub_##name##_spans, sizeof(name##_spans)); \
    gdl_shared_upload(name##_pixels, hub_##name##_pixels, sizeof(name##_pixels));
    UPLOAD_PORTAL(earth)
    UPLOAD_PORTAL(fire)
    UPLOAD_PORTAL(water)
    UPLOAD_PORTAL(air)
    markers_ready=false;
    update_markers();
    player_sprite_init();
    player_sprite_set_actions(&actions);
}
static void submit(GdlFrame *frame, const GdlSpanImage *image, int16_t x, int16_t y, uint16_t animation_frame, uint8_t priority) {
    x -= frame->camera_x; y -= frame->camera_y;
    if (x + image->width <= 0 || x >= GDL_WIDTH || y + image->height <= 0 || y >= GDL_HEIGHT) return;
    if (frame->sprite_count >= GDL_MAX_SPRITES-1) return;
    GdlSprite *sprite = &frame->sprites[frame->sprite_count++];
    *sprite = (GdlSprite){image, x, y, animation_frame, false, priority};
}
void hub_sprites_submit(GdlFrame *frame, uint32_t tick) {
    frame->sprite_count = 0;
    hub_planets_submit(frame,tick);
    for (uint16_t i = 0; i < 4; i++) if(level_progress_next(i)!=LEVEL_PORTAL_HUB) submit(frame, &portals[i], portal_positions[i][0], portal_positions[i][1], 0, GDL_SPRITE_BEHIND_FOREGROUND);
    for (uint16_t i = 0; i < 9; i++) submit(frame, &flame, flame_positions[i][0], flame_positions[i][1], (tick / 8) % 6, GDL_SPRITE_BEHIND_FOREGROUND);
    for (uint16_t i=0;i<4;i++) {
        submit(frame,&potions[i],potion_positions[i][0],potion_positions[i][1],(tick/8)%4,GDL_SPRITE_ABOVE_FOREGROUND);
        submit(frame,&hourglass,hourglass_positions[i][0],hourglass_positions[i][1],0,GDL_SPRITE_BEHIND_FOREGROUND);
    }
    update_markers();
    if(markers_visible) submit(frame,&markers,745,633,0,GDL_SPRITE_ABOVE_FOREGROUND);
    player_sprite_submit(frame, &chakan, frame->camera_x, frame->camera_y);
}
