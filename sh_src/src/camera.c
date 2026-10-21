/* Follow/deadzone subset of the original src/camera.c, with a 320x224 viewport. */
#include "camera.h"
CameraPosition cameraPosition;
static int16_t width, height;
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
