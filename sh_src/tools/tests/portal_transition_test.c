#include <assert.h>
#include <string.h>
#include "level_manager.h"
#include "level_progress.h"
#include "levels/hub/portal_hub.h"
#include "levels/hub/hub_sprites.h"
#include "levels/hub/planets.h"
#include "player.h"
#include "input_controls.h"
#include "camera.h"
#include "level_collision/collision.h"
#include "level_collision/collision_heightmaps.h"
static GameLevel song;
static unsigned bindings, fade_steps, song_changes;
void gdl_shared_upload(void *to,const void *from,uint32_t size) { memcpy(to,from,size); }
void gdl_palette_load(uint16_t first,const uint16_t *colors,uint16_t count) {
    assert(first+count<=80 && colors);
}
void gdl_palette_schedule(uint16_t first,const uint16_t *colors,uint16_t count) {
    assert(first+count<=80 && colors);
}
void gdl_palette_begin_fade(void) { fade_steps=0; }
void gdl_palette_fade_to_black(uint16_t step,uint16_t duration) {
    assert(duration==20 && step==++fade_steps);
}
bool gdl_scheduler_init(const GdlScene *scene) {
    assert(scene && scene->foreground.tiles && scene->background.tiles);
    bindings++; return true;
}
void level_music_play(GameLevel level) { song=level; song_changes++; }
static void place(int x,int y) {
    playerBody.x=x; playerBody.y=y;
    playerBody.fixX=playerBody.fixY=playerBody.velocity_x=0;
}
static void finish(GameLevel target) {
    assert(level_manager_current()==LEVEL_PORTAL_HUB);
    assert(!player_is_control_enabled() && !player_is_simulation_enabled());
    for (int i=0;i<20;i++) level_manager_update(BUTTON_UP);
    assert(level_manager_current()==LEVEL_PORTAL_HUB && fade_steps==20);
    level_manager_update(BUTTON_UP);
    assert(level_manager_current()==target && song==target);
    assert(player_is_control_enabled() && player_is_simulation_enabled());
    assert(level_manager_render_frame()->sprite_count);
    for (int i=0;i<30;i++) level_manager_update(BUTTON_UP);
    assert(level_manager_current()==target);
}
int main(void) {
    /* Two horizontal instances per type, sharing that type's vertical speed. */
    for (int i=0;i<4;i++) {
      const GdlSpanImage *image=NULL;
      for (int copy=0;copy<2;copy++) {
        const int xs[]={16,260,496,756};
        int world_x=xs[i]+copy*1048;
        int camera_x=world_x-100;
        if (camera_x<0) camera_x=0;
        if (camera_x>1600) camera_x=1600;
        GdlFrame first={.camera_x=camera_x,.camera_y=i*320};
        GdlFrame later=first;
        hub_planets_submit(&first,0); hub_planets_submit(&later,8);
        assert(first.sprite_count==1 && later.sprite_count==1);
        assert(first.sprites[0].x==world_x-camera_x && later.sprites[0].x==world_x-camera_x);
        if (copy) assert(first.sprites[0].image==image);
        image=first.sprites[0].image;
        assert(first.sprites[0].y-later.sprites[0].y==6+i*2);
        assert(later.sprites[0].behind_foreground==GDL_SPRITE_BEHIND_FOREGROUND);
      }
    }
    for (int x=0;x<=1600;x+=8) {
        GdlFrame view={.camera_x=x};
        hub_planets_submit(&view,0);
        assert(view.sprite_count<=2);
    }
    /* Leaving the viewport cannot cause a wrap. Green exits the level top
     * only after its full height, then appears at the level's bottom edge. */
    GdlFrame top={0},bottom={.camera_y=1056};
    hub_planets_submit(&top,58); hub_planets_submit(&bottom,58);
    assert(top.sprite_count==0 && bottom.sprite_count==1);
    assert(bottom.sprites[0].y==223);
    const GameLevel door_targets[]={LEVEL_PORTAL_HUB,LEVEL_T_AIR_1,LEVEL_T_EARTH_1,
        LEVEL_T_FIRE_1,LEVEL_T_WATER_1};
    /* Every source collision door ID, including unsupported final entrance. */
    for (int id=1;id<=5;id++) {
        level_manager_init();
        assert(song==LEVEL_PORTAL_HUB);
        assert(cameraPosition.y==580);
        if (id==1) {
            Player original=playerBody;
            player_set_simulation_enabled(false);
            /* Inclusive top-left region bounds, independent of jumps/height. */
            place(704,500); level_manager_update(0); assert(cameraPosition.y==580);
            place(896,800); level_manager_update(0); assert(cameraPosition.y==580);
            /* Smooth exit: normal following wants Y=709, first step is 8px. */
            place(897,800); level_manager_update(0); assert(cameraPosition.y==588);
            for (int tick=0;tick<180;tick++) level_manager_update(0);
            assert(cameraPosition.y==709);
            place(896,800); level_manager_update(0); assert(cameraPosition.y==700);
            for (int tick=0;tick<180;tick++) level_manager_update(0);
            assert(cameraPosition.y==580);
            playerBody=original;
            player_set_simulation_enabled(true);
            GdlFrame scenery={.camera_x=648,.camera_y=580};
            hub_sprites_submit(&scenery,8);
            unsigned potions=0,hourglasses=0;
            for (unsigned i=0;i<scenery.sprite_count;i++) {
                const GdlSprite *sprite=&scenery.sprites[i];
                if (sprite->image->width==16 && sprite->image->frames==4) {
                    assert(sprite->frame==1 && sprite->behind_foreground==GDL_SPRITE_ABOVE_FOREGROUND);
                    potions++;
                }
                if (sprite->image->width==24 && sprite->image->height==32) hourglasses++;
            }
            assert(potions==4 && hourglasses==4 && scenery.sprite_count>=9);
            assert(scenery.sprites[scenery.sprite_count-1].image->width==112);
        }
        int tx=-1,ty=-1;
        for (int y=0;y<80 && tx<0;y++) for (int x=0;x<120;x++)
            if (portal_hub_collision_at(x,y)==id+1) { tx=x; ty=y; break; }
        if (id==1) {
            /* The source map has no ID 1 cells; Air uses its sprite fallback.
             * Still exercise the defined door descriptor with a synthetic cell. */
            static uint8_t grid[120*80];
            assert(tx<0);
            tx=10; ty=10; grid[ty*120+tx]=2;
            collision_bind(grid,120,80,portal_hub_collision_tiles,7);
        }
        assert(tx>=0);
        /* Up pressed away from an entrance then held while arriving does nothing. */
        level_manager_update(BUTTON_UP);
        place(tx*16,ty*16);
        level_manager_update(BUTTON_UP);
        assert(player_is_control_enabled());
        level_manager_update(0);
        place(tx*16,ty*16);
        level_manager_update(BUTTON_UP);
        if (id<5) finish(door_targets[id]);
        else assert(level_manager_current()==LEVEL_PORTAL_HUB && player_is_control_enabled());
    }
    /* Actual sprite entrances also work outside the collision trigger cells. */
    static const int positions[][2]={{384,175},{1696,335},{576,1135},{1375,1103}};
    static const GameLevel targets[]={LEVEL_T_EARTH_1,LEVEL_T_FIRE_1,LEVEL_T_WATER_1,LEVEL_T_AIR_1};
    for (int i=0;i<4;i++) {
        level_manager_init();
        place(positions[i][0]-PLAYER_WIDTH+1,positions[i][1]);
        assert(hub_portal_overlap(playerBody.x,playerBody.y,PLAYER_WIDTH,PLAYER_HEIGHT)==i);
        level_manager_update(BUTTON_UP);
        finish(targets[i]);
        /* Incomplete chords do not leave the stage. Completing the chord is
         * consumed before A/C can jump, then returns through the normal loader. */
        level_manager_update(BUTTON_START|BUTTON_A);
        assert(level_manager_current()==targets[i] && player_is_control_enabled());
        level_manager_update(BUTTON_RETURN_HUB);
        assert(!player_is_simulation_enabled());
        for (int tick=0;tick<19;tick++) level_manager_update(BUTTON_RETURN_HUB);
        assert(fade_steps==20 && level_manager_current()==targets[i]);
        level_manager_update(BUTTON_RETURN_HUB);
        assert(level_manager_current()==LEVEL_PORTAL_HUB && song==LEVEL_PORTAL_HUB);
        assert(cameraPosition.y==580);
        assert(playerBody.x==756+PLAYER_OFFSET_X && !playerBody.jumping);
        for (int tick=0;tick<30;tick++) level_manager_update(BUTTON_RETURN_HUB);
        assert(level_manager_current()==LEVEL_PORTAL_HUB && player_is_control_enabled());
    }
    assert(bindings==21 && song_changes==21);
    assert(level_progress_bits()==0); /* Returning does not complete a stage. */
    /* Stage completion survives the normal fade/Hub reload, unlike abandoning
     * a level with the return chord. The next placeholder stays safely in Hub. */
    level_manager_init();
    assert(!level_manager_complete_current());
    place(384,175); level_manager_update(BUTTON_UP); finish(LEVEL_T_EARTH_1);
    assert(level_manager_complete_current());
    assert(!level_manager_complete_current());
    for(int i=0;i<21;i++) level_manager_update(0);
    assert(level_manager_current()==LEVEL_PORTAL_HUB);
    assert(level_progress_stage(0,0,0));
    assert(level_progress_next(0)==LEVEL_T_EARTH_2);
    assert(!level_manager_available(LEVEL_T_EARTH_2));
    place(384,175); level_manager_update(BUTTON_UP);
    for(int i=0;i<25;i++) level_manager_update(0);
    assert(level_manager_current()==LEVEL_PORTAL_HUB && player_is_control_enabled());
    /* One combined marker sprite preserves all 24 indicators and player. */
    for(int id=1;id<LEVEL_FINAL;id++) assert(level_progress_mark((GameLevel)id));
    assert(level_progress_realm(0) && level_progress_realm(1));
    assert(level_progress_next(0)==LEVEL_PORTAL_HUB);
    assert(hub_portal_overlap(384,175,42,58)==-1);
    GdlFrame markers={.camera_x=648,.camera_y=580};
    hub_sprites_submit(&markers,8);
    unsigned found=0;
    for(unsigned i=0;i<markers.sprite_count;i++) if(markers.sprites[i].image->width==115) {
        const GdlSpanImage *image=markers.sprites[i].image;
        assert(image->height==115 && image->row_offsets[115]==200 && image->row_offsets[230]==400);
        for(unsigned row=0;row<230;row++) for(unsigned run=image->row_offsets[row];run<image->row_offsets[row+1];run++) {
            assert(image->spans[run*3]+image->spans[run*3+1]<=115);
        }
        found++;
    }
    assert(found==1 && markers.sprite_count<=GDL_MAX_SPRITES);
    assert(markers.sprites[markers.sprite_count-1].image->width==112);
    static uint8_t final_grid[120*80];
    final_grid[10*120+10]=3; /* Door descriptor 2 (Earth). */
    collision_bind(final_grid,120,80,portal_hub_collision_tiles,7);
    portal_hub_update(0); place(160,160); portal_hub_update(BUTTON_UP);
    assert(portal_hub_take_destination()==LEVEL_FINAL);
    assert(!level_manager_available(LEVEL_FINAL));
    level_manager_init();
    assert(level_progress_bits()==0);

    return 0;
}
