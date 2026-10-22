#include <assert.h>
#include <string.h>
#include <stdio.h>
#include "level_collision/collision.h"
#include "level_collision/collision_heightmaps.h"
#include "input_controls.h"
#include "player_sprite.h"
static uint8_t cells[40 * 12];
static void reset(void) {
    memset(cells,0,sizeof(cells));
    collision_bind(cells,40,12,te1_collision_tiles,10);
}
static void fill_row(int row,int tile) { for (int x=0;x<40;x++) cells[row*40+x]=tile; }
static void spawn(int x,int feet) {
    playerInit(x,feet-PLAYER_HEIGHT,collision_blocked);
    player_set_collision_move(collision_move_player);
    player_set_collision_queries(collision_can_drop,collision_can_stand);
    input_init();
}
static void tick(int pad) { processInput(pad); updatePlayer(); }
int main(void) {
    reset(); fill_row(5,8); fill_row(8,1); spawn(64,80);
    tick(BUTTON_DOWN);
    assert(playerBody.crouching && playerBody.height==40 && playerBody.y+playerBody.height==80);
    tick(BUTTON_DOWN|BUTTON_A);
    assert(!playerBody.onGround && playerBody.drop_timer==11 && !playerBody.jumping && !playerBody.rolling);
    for (int i=0;i<40;i++) tick(BUTTON_DOWN|BUTTON_A);
    assert(playerBody.onGround && playerBody.y+playerBody.height==128 && !playerBody.drop_timer);
    /* Holding jump never repeats the drop or starts a second action. */
    assert(!playerBody.rolling);
    reset(); fill_row(5,8); cells[5*40+5]=1; spawn(64,80);
    assert(!player_request_drop_through());
    reset(); fill_row(5,2); spawn(64,80);
    assert(!player_request_drop_through()); /* Floor-only is not explicitly one-way. */
    /* One-way undersides do not cancel an ascending jump. */
    reset(); fill_row(5,8); spawn(64,130);
    collision_move_player(0,-12);
    assert(playerBody.y==130-PLAYER_HEIGHT-12 && !playerBody.onGround);
    /* Fast falls cross a platform even when the final feet penetrate >1px. */
    spawn(64,77); collision_move_player(0,13);
    assert(playerBody.onGround && playerBody.y+playerBody.height==80);
    /* Crouch keeps feet fixed and cannot stand into a low ceiling. */
    reset(); fill_row(6,1); spawn(64,96); tick(BUTTON_DOWN);
    fill_row(2,1); tick(0);
    assert(playerBody.crouching && playerBody.height==40 && playerBody.y+40==96);
    fill_row(2,0); tick(0);
    assert(!playerBody.crouching && playerBody.height==58 && playerBody.y+58==96);
    /* Solid-ground DOWN+jump rolls, bounded to 137px; release cancels. */
    reset(); fill_row(8,1); spawn(64,128);
    tick(BUTTON_DOWN|BUTTON_A);
    assert(playerBody.rolling && playerBody.velocity_x==8 && playerBody.height==40);
    for (int i=0;i<25;i++) tick(BUTTON_DOWN|BUTTON_A);
    assert(!playerBody.rolling && playerBody.x==64+137);
    tick(0); tick(BUTTON_DOWN|BUTTON_A); assert(playerBody.rolling);
    tick(BUTTON_DOWN); assert(!playerBody.rolling && !playerBody.velocity_x);
    /* No tunneling through solid walls at roll speed. */
    reset(); fill_row(8,1); spawn(64,128);
    for (int y=0;y<8;y++) cells[y*40+8]=1;
    for (int i=0;i<20;i++) tick(BUTTON_DOWN|BUTTON_A);
    assert(playerBody.x+PLAYER_WIDTH<=128);
    player_set_control_enabled(false);
    assert(!playerBody.rolling && !playerBody.drop_timer && !playerBody.input_y);
    assert(!player_request_drop_through() && !player_request_roll());
    /* Preserve all source slope samples. */
    for (int x=0;x<16;x++) {
        assert(te1_collision_heightmaps[4][x]==1+x/2);
        assert(te1_collision_heightmaps[5][x]==9+x/2);
        assert(te1_collision_heightmaps[6][x]==16-x/2);
        assert(te1_collision_heightmaps[7][x]==8-x/2);
    }
    reset(); cells[5*40+5]=4; cells[5*40+6]=5;
    spawn(64,94); playerBody.onGround=true;
    for (int i=0;i<24;i++) { collision_move_player(1,0); assert(playerBody.onGround); }
    for (int i=0;i<24;i++) { collision_move_player(-1,0); assert(playerBody.onGround); }
    /* Ceiling correction and bounds preserve collider dimensions. */
    reset(); fill_row(2,1); spawn(64,110); collision_move_player(0,-12);
    assert(playerBody.y==48 && !playerBody.onGround);
    assert(player_collision_result.flags & COLLISION_CEILING);
    collision_move_player(-100,0); assert(playerBody.x==0);
    /* Source corner nudge clears a inset corner snag without cancelling ascent. */
    reset(); cells[2*40+4]=1; spawn(78,111); collision_move_player(0,-12);
    assert(playerBody.x==79 && playerBody.y==41);
    assert(!(player_collision_result.flags & COLLISION_CEILING));
    /* Action sprite selection preserves the feet/sprite origin when crouching. */
    reset(); fill_row(8,1); spawn(64,128);
    GdlSpanImage normal={0}, actions={0}; GdlFrame frame={0};
    player_sprite_init(); player_sprite_set_actions(&actions);
    tick(BUTTON_DOWN); player_sprite_submit(&frame,&normal,0,0);
    assert(frame.sprites[0].image==&actions && frame.sprites[0].frame==0);
    assert(frame.sprites[0].y==48);
    tick(BUTTON_DOWN|BUTTON_A);
    for (int i=0;i<12;i++) {
        frame.sprite_count=0; player_sprite_submit(&frame,&normal,0,0);
        assert(frame.sprites[0].image==&actions && frame.sprites[0].frame>=1 && frame.sprites[0].frame<=4);
    }
    /* All five Hub markers report door ID/param and never block. */
    for (int id=1;id<=5;id++) {
        reset(); cells[4*40+5]=id+1;
        collision_bind(cells,40,12,portal_hub_collision_tiles,7);
        spawn(80,100);
        const TileBehaviorData *b=collision_player_behavior(TILE_BEHAVIOR_DOOR);
        assert(b && b->id==id && b->param==0);
        tick(0); assert(player_collision_result.specialType==TILE_BEHAVIOR_DOOR);
    }
    puts("Crouch, headroom, drop-through, roll, slopes, ceiling and door checks passed");
    return 0;
}
