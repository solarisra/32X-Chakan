#include <assert.h>
#include <string.h>
#include "levels/terrestrial/fire/t_fire_1.h"
#include "player.h"
#include "input_controls.h"
#include "level_collision/collision.h"
#include "level_collision/collision_heightmaps.h"
static const GdlScene *bound_scene;
void gdl_shared_upload(void *to,const void *from,uint32_t size) { memcpy(to,from,size); }
void gdl_palette_load(uint16_t first,const uint16_t *colors,uint16_t count) {
    assert(first==0 && count==48); (void)colors;
}
void gdl_palette_schedule(uint16_t first,const uint16_t *colors,uint16_t count) {
    assert(first==0 && count==48); (void)colors;
}
bool gdl_scheduler_init(const GdlScene *scene) { bound_scene=scene; return scene!=0; }
int main(void) {
    t_fire1_init();
    assert(playerBody.x==256+35 && playerBody.y==100+22);
    assert(bound_scene->foreground.width_tiles==200 && bound_scene->foreground.height_tiles==232);
    assert(bound_scene->background.width_tiles==1 && bound_scene->background.height_tiles==1);
    assert(bound_scene->background.map[0]==0);
    for (int i=0;i<16;i++) assert(bound_scene->background.tiles[i]==0);
    player_set_simulation_enabled(false);
    const GdlFrame *f=t_fire1_render_frame();
    for (unsigned tick=0;tick<180;tick++) {
        if (tick==30) { playerBody.x=1500; playerBody.y=1800; }
        t_fire1_update(0);
        assert(!f->background_per_row && !f->background_per_line);
        assert(!f->background_x && !f->background_y && !f->background_priority);
        assert(f->foreground_priority && f->sprites[0].behind_foreground==GDL_SPRITE_BEHIND_PRIORITY);
        assert(f->camera_x>=0 && f->camera_x<=1280 && f->camera_y>=0 && f->camera_y<=1632);
    }
    assert(f->camera_x==1280 && f->camera_y==1632);
    for (int id=0;id<8;id++) {
        bool one_way=id>=3 && id<=5;
        assert(!!(tf1_collision_tiles[id]->geometry->flags&TILE_FLAG_ONE_WAY)==one_way);
    }
    assert(tf1_collision_tiles[0]->geometry->shape==TILE_SHAPE_EMPTY);
    assert(tf1_collision_tiles[7]->geometry->shape==TILE_SHAPE_FULL);
    assert(tf1_collision_tiles[2]->geometry->flags==FLAGS_FLOOR);
    assert(tf1_collision_tiles[6]->geometry->flags==FLAGS_FLOOR);
    for (int x=0;x<16;x++) {
        assert(tf1_collision_tiles[2]->geometry->hmap[x]==8);
        assert(tf1_collision_tiles[6]->geometry->hmap[x]==8);
        assert(tf1_collision_tiles[4]->geometry->hmap[x]==(x<8?0:8));
        assert(tf1_collision_tiles[5]->geometry->hmap[x]==(x<8?8:0));
    }
    /* Original bridge heights: land at cell Y+8, drop only from IDs 3..5. */
    uint8_t grid[20*12]={0};
    for (int x=0;x<20;x++) { grid[5*20+x]=3; grid[9*20+x]=1; }
    collision_bind(grid,20,12,tf1_collision_tiles,8);
    playerInit(64,88-PLAYER_HEIGHT,collision_blocked);
    player_set_collision_move(collision_move_player);
    player_set_collision_queries(collision_can_drop,collision_can_stand);
    input_init();
    assert(playerBody.onGround && player_request_drop_through());
    for (int i=0;i<40;i++) updatePlayer();
    assert(playerBody.onGround && playerBody.y+playerBody.height==144);
    for (int x=0;x<20;x++) grid[5*20+x]=2;
    playerInit(64,88-PLAYER_HEIGHT,collision_blocked);
    assert(playerBody.onGround && !player_request_drop_through());
    return 0;
}
