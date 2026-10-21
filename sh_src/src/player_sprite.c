#include "player_sprite.h"
#include "player.h"
static uint16_t animation, walking_frame, ticks;
void player_sprite_init(void) { animation = walking_frame = ticks = 0; }
void player_sprite_submit(GdlFrame *frame, const GdlSpanImage *image, int16_t camera_x, int16_t camera_y) {
    uint16_t next = playerBody.onGround ? (playerBody.velocity_x ? 1 : 0) : (playerBody.fixY < 0 ? 2 : 3);
    if (playerBody.onGround && animation == 3) next = 4;
    if (animation == 4 && playerBody.onGround && ticks < 8) next = 4;
    if (next != animation) { animation = next; walking_frame = ticks = 0; }
    else if (++ticks >= 8 && animation == 1) { walking_frame = (walking_frame + 1) & 7; ticks = 0; }
    if (frame->sprite_count >= GDL_MAX_SPRITES) return;
    GdlSprite *sprite = &frame->sprites[frame->sprite_count++];
    sprite->image = image;
    sprite->x = playerBody.x - PLAYER_OFFSET_X - camera_x;
    sprite->y = playerBody.y - PLAYER_OFFSET_Y - camera_y;
    sprite->flip = playerBody.facingDirection > 0;
    sprite->behind_foreground = false;
    sprite->frame = animation == 0 ? 0 : animation == 1 ? 1 + walking_frame : animation + 7;
}
