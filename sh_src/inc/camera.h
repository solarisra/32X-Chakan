#ifndef CHAKAN_SH_CAMERA_H
#define CHAKAN_SH_CAMERA_H
#include <stdint.h>
typedef struct { int16_t x, y; } CameraPosition;
extern CameraPosition cameraPosition;
void camera_init(int16_t level_w, int16_t level_h);
void camera_step(int16_t center_x, int16_t center_y);
#endif
