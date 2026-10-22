#include <assert.h>
#include <string.h>
#include "levels/terrestrial/air/t_air_1.h"
#include "player.h"
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
    t_air1_init();
    assert(playerBody.x==464+35 && playerBody.y==64+22);
    player_set_simulation_enabled(false);
    const GdlFrame *f=t_air1_render_frame();
    for (unsigned tick=1;tick<=513;tick++) {
        t_air1_update(0);
        assert(f->background_per_row && f->background_y==0 && f->foreground_priority);
        for (int row=0;row<28;row++) {
            unsigned speed=row<=10?(11-row)*64:row>=17?(row-16)*64:0;
            unsigned phase=(tick*speed)&65535;
            assert(f->background_rows[row]==((f->camera_x/8-(phase>>8))&255));
        }
        assert(f->sprites[0].behind_foreground==2);
    }
    t_air1_init(); player_set_simulation_enabled(false); t_air1_update(0);
    assert(f->background_rows[0]==((f->camera_x/8-2)&255));
    /* Air-specific 9h tiles block on every side; ID 4 stays empty. */
    assert(ta1_collision_tiles[2]->geometry->hmap[0]==9);
    assert(ta1_collision_tiles[2]->geometry->flags==FLAGS_SOLID);
    assert(ta1_collision_tiles[4]->geometry->shape==TILE_SHAPE_EMPTY);
    uint8_t grid[20*12]={0};
    for (int x=0;x<20;x++) grid[5*20+x]=2;
    collision_bind(grid,20,12,ta1_collision_tiles,5);
    playerInit(64,80-PLAYER_HEIGHT,collision_blocked);
    collision_move_player(0,13);
    assert(playerBody.onGround && playerBody.y+playerBody.height==87);
    assert(!collision_can_drop());
    playerInit(64,100,collision_blocked);
    collision_move_player(0,-10);
    assert(player_collision_result.flags & COLLISION_CEILING);
    assert(playerBody.y==96);
    return 0;
}
