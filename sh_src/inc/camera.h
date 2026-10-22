#ifndef CHAKAN_SH_CAMERA_H
#define CHAKAN_SH_CAMERA_H
#include <stdint.h>
#include <stdbool.h>
typedef struct { int16_t x, y; } CameraPosition;
typedef struct { int16_t min_x,max_x,min_y,max_y; } CameraRegion;
extern CameraPosition cameraPosition;
void camera_init(int16_t level_w, int16_t level_h);
void camera_step(int16_t center_x, int16_t center_y);
void camera_set_y(int16_t y);
/* Source regions test the player's top-left point, with inclusive edges. */
bool camera_in_region(int16_t x,int16_t y,const CameraRegion *region);
void camera_apply_y_region_lock(int16_t previous_y,int16_t player_x,int16_t player_y,
                               const CameraRegion *region,int16_t locked_y,
                               uint8_t smooth_shift,bool *transition_active);
#endif
