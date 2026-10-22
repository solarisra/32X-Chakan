#include <assert.h>
#include <string.h>
#include "levels/terrestrial/water/t_water_1.h"
#include "player.h"
#include "input_controls.h"
#include "level_collision/collision.h"
#include "level_collision/collision_heightmaps.h"
void gdl_shared_upload(void *to,const void *from,uint32_t size) { memcpy(to,from,size); }
void gdl_palette_load(uint16_t first,const uint16_t *colors,uint16_t count) {
    (void)first; (void)colors; (void)count;
}
void gdl_palette_schedule(uint16_t first,const uint16_t *colors,uint16_t count) {
    (void)first; (void)colors; (void)count;
}
bool gdl_scheduler_init(const GdlScene *scene) { return scene!=0; }
int main(void) {
    t_water1_init();
    assert(playerBody.x==8+35 && playerBody.y==250+22);
    player_set_simulation_enabled(false);
    const GdlFrame *f=t_water1_render_frame();
    unsigned previous_underwater_drift=0, underwater_ticks=0;
    for (unsigned tick=1;tick<=513;tick++) {
        if (tick==200) { playerBody.x=2000; playerBody.y=400; }
        t_water1_update(0);
        assert(f->background_per_line && f->foreground_priority && f->background_priority);
        assert(f->background_pixel_priority && f->foreground_map_priority);
        assert(f->background_y==f->camera_y);
        for (int y=0;y<GDL_HEIGHT;y++) {
            unsigned world=f->camera_y+y,drift=0;
            if (world>=384) drift=((tick*640)&131071)>>8;
            else if (world>=289) {
                unsigned speed=128+(world-289)*384/94;
                drift=((tick*speed)&131071)>>8;
            }
            assert(f->background_rows[y]==((f->camera_x/2+drift)&511));
        }
        assert(f->sprites[0].behind_foreground==GDL_SPRITE_BEHIND_PRIORITY);
        /* The entire underwater band is rigid, including across image wrap.
         * Remove camera parallax before checking the source 640/256px cadence. */
        int first_underwater=384-f->camera_y;
        if (first_underwater<0) first_underwater=0;
        if (first_underwater<GDL_HEIGHT) {
            unsigned scroll=f->background_rows[first_underwater];
            for (int y=first_underwater+1;y<GDL_HEIGHT;y++)
                assert(f->background_rows[y]==scroll);
            unsigned drift=(scroll-f->camera_x/2)&511;
            if (underwater_ticks) {
                unsigned step=(drift-previous_underwater_drift)&511;
                assert(step==((tick*640)>>8)-(((tick-1)*640)>>8));
            }
            previous_underwater_drift=drift;
            underwater_ticks++;
        }
    }
    assert(underwater_ticks>254); /* Includes at least one 512px phase wrap. */
    /* Only the eight source bridge profiles carry ONE_WAY. */
    for (int id=0;id<27;id++) {
        bool expected=(id>=8 && id<=11)||(id>=15 && id<=18);
        assert(!!(tw1_collision_tiles[id]->geometry->flags & TILE_FLAG_ONE_WAY)==expected);
    }
    assert(tw1_collision_tiles[2]->geometry->hmap[0]==9);
    assert(tw1_collision_tiles[2]->geometry->flags==FLAGS_FLOOR);
    assert(tw1_collision_tiles[13]->geometry->hmap[13]==16);
    assert(tw1_collision_tiles[13]->geometry->hmap[14]==0);
    uint8_t grid[20*12]={0};
    for (int x=0;x<20;x++) { grid[5*20+x]=9; grid[9*20+x]=1; }
    collision_bind(grid,20,12,tw1_collision_tiles,27);
    playerInit(64,80-PLAYER_HEIGHT,collision_blocked);
    player_set_collision_move(collision_move_player);
    player_set_collision_queries(collision_can_drop,collision_can_stand);
    input_init();
    assert(playerBody.onGround && player_request_drop_through());
    for (int i=0;i<40;i++) updatePlayer();
    assert(playerBody.onGround && playerBody.y+playerBody.height==144);
    grid[9*20+5]=3;
    assert(!player_request_drop_through());
    return 0;
}
