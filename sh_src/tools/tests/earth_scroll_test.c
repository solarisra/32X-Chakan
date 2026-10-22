#include <assert.h>
#include <string.h>
#include "levels/terrestrial/earth_levels.h"
#include "player.h"
#include "camera.h"
/* Host substitutes only for hardware upload/palette/scheduler calls. */
void gdl_shared_upload(void *to, const void *from, uint32_t size) { memcpy(to,from,size); }
void gdl_palette_load(uint16_t first,const uint16_t *colors,uint16_t count) {
    (void)first; (void)colors; (void)count;
}
void gdl_palette_schedule(uint16_t first,const uint16_t *colors,uint16_t count) {
    (void)first; (void)colors; (void)count;
}
bool gdl_scheduler_init(const GdlScene *scene) { return scene!=0; }
int main(void) {
    t_earth1_init();
    player_set_simulation_enabled(false);
    t_earth1_update(0);
    const GdlFrame *f=t_earth1_render_frame();
    assert(f->camera_x==0 && f->background_x==0);
    playerBody.x+=2; t_earth1_update(0);
    assert(f->camera_x==0 && f->background_x==0); /* Inside dead zone. */
    playerBody.x=200; t_earth1_update(0);
    assert(f->camera_x==57 && f->background_x==28);
    playerBody.x+=1; t_earth1_update(0);
    assert(f->camera_x==58 && f->background_x==29); /* Retains half pixels. */
    playerBody.x-=1; t_earth1_update(0);
    assert(f->camera_x==58 && f->background_x==29);
    playerBody.x=2500; t_earth1_update(0);
    assert(f->camera_x==2256);
    uint16_t before=f->background_x;
    playerBody.x+=2; t_earth1_update(0);
    assert(f->camera_x==2256 && f->background_x==before); /* Camera clamped. */
    playerBody.x=0; t_earth1_update(0);
    assert(f->camera_x==0 && f->background_x==0); /* Reverse travel. */
    int old_camera_y=f->camera_y;
    uint16_t old_bg_y=f->background_y;
    playerBody.y+=50; t_earth1_update(0);
    assert(f->background_y==(old_bg_y+f->camera_y-old_camera_y)%224);
    before=f->background_x;
    uint16_t y=f->background_y;
    for (int i=0;i<20;i++) t_earth1_update(0);
    assert(f->background_x==before && f->background_y==y);
    return 0;
}
