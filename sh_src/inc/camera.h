#ifndef CHAKAN_SH_CAMERA_H
#define CHAKAN_SH_CAMERA_H
#include <stdint.h>
#include <stdbool.h>
#include "gdl/renderer.h"
#include "level_collision/collision.h"
typedef struct { int16_t x, y; } CameraPosition;
typedef struct { int16_t min_x,max_x,min_y,max_y; } CameraRegion;
typedef enum { CAM_MODE_FOLLOW, CAM_MODE_CINEMATIC } CameraMode;
/* Published render position includes shake. Read with camera_get_pos(); move
 * the logical camera with the setters rather than writing this global. */
extern CameraPosition cameraPosition;
void camera_init(int16_t level_w,int16_t level_h);
/* Keep the port's existing 156..164 / 104..120 default deadzone. */
void camera_set_deadzone(int16_t min_x,int16_t min_y,int16_t max_x,int16_t max_y);
void camera_set_lookahead(int16_t ahead_x,int16_t ahead_y);
void camera_set_lookahead_smoothing(uint8_t shift);
void camera_set_follow(void);
void camera_set_cinematic_center(int16_t center_x,int16_t center_y,uint8_t smooth_shift);
void camera_set_position(int16_t x,int16_t y);
void camera_set_y(int16_t y);
void camera_approach_y(int16_t y,uint8_t smooth_shift);
void camera_shake(uint8_t magnitude,uint16_t duration);
/* Zero-velocity compatibility wrapper, and velocity-aware per-tick update. */
void camera_step(int16_t center_x,int16_t center_y);
void camera_step_velocity(int16_t center_x,int16_t center_y,int16_t velocity_x,int16_t velocity_y);
CameraPosition camera_get_pos(void);
/* MarsDev equivalents of the SGDK Map/VDP helpers. Background offsets wrap to
 * the supplied map. Zero parallax denominator leaves the frame unchanged. */
void camera_apply_foreground(GdlFrame *frame);
void camera_apply_parallax(GdlFrame *frame,const GdlTilemap *background,int16_t num,int16_t den);
/* Rate uses SGDK fix16 units: 64 == one pixel/tick. Call after parallax. */
void camera_apply_bg_b_const_scroll_y(GdlFrame *frame,const GdlTilemap *background,int16_t rate);
/* Source regions test the player's top-left point, with inclusive edges. */
bool camera_in_region(int16_t x,int16_t y,const CameraRegion *region);
bool camera_check_bounds_in_region(const AABB *bounds,const CameraRegion *region);
int16_t camera_get_region_locked_y(int16_t normal_y,const AABB *bounds,const CameraRegion *region,int16_t locked_y);
void camera_apply_y_region_lock(int16_t previous_y,int16_t player_x,int16_t player_y,
                               const CameraRegion *region,int16_t locked_y,
                               uint8_t smooth_shift,bool *transition_active);
#endif
