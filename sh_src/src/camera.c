/* Follow/deadzone subset of the original src/camera.c, with a 320x224 viewport. */
#include "camera.h"
CameraPosition cameraPosition;
static int16_t width, height;
void camera_set_y(int16_t y) {
    int16_t maximum=height>224?height-224:0;
    cameraPosition.y=y<0?0:y>maximum?maximum:y;
}
bool camera_in_region(int16_t x,int16_t y,const CameraRegion *region) {
    return x>=region->min_x && x<=region->max_x && y>=region->min_y && y<=region->max_y;
}
void camera_apply_y_region_lock(int16_t previous_y,int16_t player_x,int16_t player_y,
                               const CameraRegion *region,int16_t locked_y,
                               uint8_t smooth_shift,bool *transition_active) {
    bool inside=camera_in_region(player_x,player_y,region);
    int16_t normal_y=cameraPosition.y;
    if (!inside && !*transition_active) return;
    int16_t target=inside?locked_y:normal_y;
    int32_t delta=target-previous_y, step=delta;
    if (smooth_shift) {
        if (smooth_shift>15) smooth_shift=15;
        int32_t divisor=1L<<smooth_shift;
        /* Match SGDK's arithmetic right shift, including negative rounding. */
        step=delta<0?-((-delta+divisor-1)/divisor):delta/divisor;
        if (!step && delta) step=delta>0?1:-1;
    }
    camera_set_y(previous_y+step);
    *transition_active=inside || cameraPosition.y!=normal_y;
}
void camera_init(int16_t w, int16_t h) {
    cameraPosition = (CameraPosition){0, 0};
    width = w;
    height = h;
}
void camera_step(int16_t x, int16_t y) {
    int16_t sx = x - cameraPosition.x, sy = y - cameraPosition.y;
    if (sx > 164) cameraPosition.x += sx - 164;
    else if (sx < 156) cameraPosition.x += sx - 156;
    if (sy > 120) cameraPosition.y += sy - 120;
    else if (sy < 104) cameraPosition.y += sy - 104;
    if (cameraPosition.x < 0) cameraPosition.x = 0;
    if (cameraPosition.y < 0) cameraPosition.y = 0;
    if (cameraPosition.x > width - 320) cameraPosition.x = width - 320;
    if (cameraPosition.y > height - 224) cameraPosition.y = height - 224;
}
