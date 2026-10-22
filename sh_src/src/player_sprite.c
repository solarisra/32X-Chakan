#include "player_sprite.h"
#include "player.h"
static uint16_t animation, walking_frame, ticks;
static const GdlSpanImage *actions;
void player_sprite_init(void) { animation = walking_frame = ticks = 0; actions = 0; }
void player_sprite_set_actions(const GdlSpanImage *image) { actions = image; }
void player_sprite_submit(GdlFrame *frame, const GdlSpanImage *image, int16_t camera_x, int16_t camera_y) {
    uint16_t next = playerBody.onGround ? (playerBody.velocity_x ? 1 : 0) : (playerBody.fixY < 0 ? 2 : 3);
    if (playerBody.onGround && playerBody.crouching) next=5;
    if (playerBody.rolling) next=6;
    if (next != 6 && playerBody.onGround && animation == 3) next = 4;
    if (next != 6 && animation == 4 && playerBody.onGround && ticks < 8) next = 4;
    if (next != animation) { animation = next; walking_frame = ticks = 0; }
    else if (++ticks >= 8 && animation == 1) { walking_frame = (walking_frame + 1) & 7; ticks = 0; }
    if (animation == 6 && ticks >= 3) { walking_frame=(walking_frame+1)&3; ticks=0; }
    if (frame->sprite_count >= GDL_MAX_SPRITES) return;
    GdlSprite *sprite = &frame->sprites[frame->sprite_count++];
    sprite->image = image;
    sprite->x = playerBody.x - PLAYER_OFFSET_X - camera_x;
    sprite->y = playerBody.y - (80-playerBody.height) - camera_y;
    sprite->flip = playerBody.facingDirection > 0;
    sprite->behind_foreground = false;
    sprite->frame = animation == 0 ? 0 : animation == 1 ? 1 + walking_frame : animation + 7;
    if (animation >= 5 && !actions) sprite->frame=0;
    if (animation >= 5 && actions) {
        sprite->image=actions;
        sprite->frame=animation==5?0:1+walking_frame;
    }
}
