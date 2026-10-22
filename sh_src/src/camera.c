/* Portable camera controls adapted from SGDK create_level_stages. */
#include "camera.h"
CameraPosition cameraPosition;
static struct {
 CameraPosition position,cinematic,ahead,maximum;
 CameraRegion deadzone;
 int16_t width,height;
 uint16_t shake_timer;
 uint8_t cinematic_shift,ahead_shift,shake;
 CameraMode mode;
 int32_t background_phase;
} cam;
static int16_t clamp(int32_t value,int16_t extent,int16_t viewport) {
 int32_t maximum=extent>viewport?extent-viewport:0;
 return value<0?0:value>maximum?(int16_t)maximum:(int16_t)value;
}
static void publish(void) { cameraPosition=cam.position; }
/* Explicit floor division matches SGDK arithmetic shifts on either host. */
static int32_t shifted(int32_t value,uint8_t shift) {
 if(shift>15) shift=15;
 int32_t divisor=1L<<shift;
 return value<0?-((-value+divisor-1)/divisor):value/divisor;
}
static int16_t wrap(int32_t value,int32_t size) {
 if(size<=0) return 0;
 value%=size; if(value<0) value+=size;
 return (int16_t)value;
}
void camera_set_position(int16_t x,int16_t y) {
 cam.position.x=clamp(x,cam.width,GDL_WIDTH);
 cam.position.y=clamp(y,cam.height,GDL_HEIGHT);
 publish();
}
void camera_set_y(int16_t y) {
 cam.position.y=clamp(y,cam.height,GDL_HEIGHT); cameraPosition.y=cam.position.y;
}
void camera_approach_y(int16_t y,uint8_t shift) {
 int32_t delta=(int32_t)y-cam.position.y,step=shifted(delta,shift);
 if(!step && delta) step=delta>0?1:-1;
 cam.position.y=clamp(cam.position.y+step,cam.height,GDL_HEIGHT);
 cameraPosition.y=cam.position.y;
}
void camera_init(int16_t width,int16_t height) {
 cam.width=width; cam.height=height; cam.mode=CAM_MODE_FOLLOW;
 cam.cinematic=cam.ahead=cam.maximum=(CameraPosition){0,0};
 cam.deadzone=(CameraRegion){156,164,104,120};
 cam.cinematic_shift=0; cam.ahead_shift=2;
 cam.shake=0; cam.shake_timer=0; cam.background_phase=0;
 camera_set_position(0,0);
}
void camera_set_deadzone(int16_t min_x,int16_t min_y,int16_t max_x,int16_t max_y) {
 if(min_x>max_x || min_y>max_y) return;
 cam.deadzone=(CameraRegion){min_x,max_x,min_y,max_y};
}
void camera_set_lookahead(int16_t x,int16_t y) {
 /* Magnitudes are nonnegative; avoid overflow for INT16_MIN. */
 cam.maximum.x=x<0?0:x; cam.maximum.y=y<0?0:y;
 if(!cam.maximum.x) cam.ahead.x=0;
 if(!cam.maximum.y) cam.ahead.y=0;
}
void camera_set_lookahead_smoothing(uint8_t shift) { cam.ahead_shift=shift>15?15:shift; }
void camera_set_follow(void) { cam.mode=CAM_MODE_FOLLOW; }
void camera_set_cinematic_center(int16_t x,int16_t y,uint8_t shift) {
 cam.mode=CAM_MODE_CINEMATIC;
 cam.cinematic.x=clamp((int32_t)x-GDL_WIDTH/2,cam.width,GDL_WIDTH);
 cam.cinematic.y=clamp((int32_t)y-GDL_HEIGHT/2,cam.height,GDL_HEIGHT);
 cam.cinematic_shift=shift>15?15:shift;
 cam.ahead=(CameraPosition){0,0};
}
void camera_shake(uint8_t magnitude,uint16_t duration) { cam.shake=magnitude; cam.shake_timer=duration; }
static int16_t shake_component(uint16_t tick,uint8_t magnitude) {
 int32_t noise=(((uint32_t)tick*37)^((uint32_t)tick*19))&15;
 return (int16_t)shifted((noise-8)*magnitude,3);
}
void camera_step_velocity(int16_t x,int16_t y,int16_t vx,int16_t vy) {
 int32_t next_x=cam.position.x,next_y=cam.position.y;
 if(cam.mode==CAM_MODE_CINEMATIC) {
  next_x+=shifted((int32_t)cam.cinematic.x-next_x,cam.cinematic_shift);
  next_y+=shifted((int32_t)cam.cinematic.y-next_y,cam.cinematic_shift);
 } else {
  int32_t ax=vx>0?cam.maximum.x:vx<0?-cam.maximum.x:0;
  int32_t ay=vy>0?cam.maximum.y:vy<0?-cam.maximum.y:0;
  cam.ahead.x+=shifted(ax-cam.ahead.x,cam.ahead_shift);
  cam.ahead.y+=shifted(ay-cam.ahead.y,cam.ahead_shift);
  int32_t sx=(int32_t)x+cam.ahead.x-next_x,sy=(int32_t)y+cam.ahead.y-next_y;
  if(sx>cam.deadzone.max_x) next_x+=sx-cam.deadzone.max_x;
  else if(sx<cam.deadzone.min_x) next_x+=sx-cam.deadzone.min_x;
  if(sy>cam.deadzone.max_y) next_y+=sy-cam.deadzone.max_y;
  else if(sy<cam.deadzone.min_y) next_y+=sy-cam.deadzone.min_y;
 }
 cam.position.x=clamp(next_x,cam.width,GDL_WIDTH);
 cam.position.y=clamp(next_y,cam.height,GDL_HEIGHT);
 next_x=cam.position.x; next_y=cam.position.y;
 if(cam.shake_timer && cam.shake) {
  next_x+=shake_component(cam.shake_timer,cam.shake);
  next_y+=shake_component(cam.shake_timer^0x55,cam.shake);
  if(!--cam.shake_timer) cam.shake=0;
 }
 /* Unlike the VDP, the software renderer requires a valid viewport at edges. */
 cameraPosition.x=clamp(next_x,cam.width,GDL_WIDTH);
 cameraPosition.y=clamp(next_y,cam.height,GDL_HEIGHT);
}
void camera_step(int16_t x,int16_t y) { camera_step_velocity(x,y,0,0); }
CameraPosition camera_get_pos(void) { return cameraPosition; }
void camera_apply_foreground(GdlFrame *frame) {
 if(frame) { frame->camera_x=cameraPosition.x; frame->camera_y=cameraPosition.y; }
}
void camera_apply_parallax(GdlFrame *frame,const GdlTilemap *background,int16_t num,int16_t den) {
 if(!frame || !background || !den) return;
 frame->background_x=(uint16_t)wrap((int32_t)cameraPosition.x*num/den,(int32_t)background->width_tiles*8);
 frame->background_y=(uint16_t)wrap((int32_t)cameraPosition.y*num/den,(int32_t)background->height_tiles*8);
}
void camera_apply_bg_b_const_scroll_y(GdlFrame *frame,const GdlTilemap *background,int16_t rate) {
 if(!frame || !background || !background->height_tiles) return;
 int32_t size=(int32_t)background->height_tiles*8*64;
 cam.background_phase=(cam.background_phase+rate)%size;
 if(cam.background_phase<0) cam.background_phase+=size;
 frame->background_y=(uint16_t)(cam.background_phase/64);
}
bool camera_in_region(int16_t x,int16_t y,const CameraRegion *region) {
 return region && x>=region->min_x && x<=region->max_x && y>=region->min_y && y<=region->max_y;
}
bool camera_check_bounds_in_region(const AABB *bounds,const CameraRegion *region) {
 return bounds && camera_in_region(bounds->min.x,bounds->min.y,region);
}
int16_t camera_get_region_locked_y(int16_t normal_y,const AABB *bounds,const CameraRegion *region,int16_t locked_y) {
 return camera_check_bounds_in_region(bounds,region)?locked_y:normal_y;
}
void camera_apply_y_region_lock(int16_t previous_y,int16_t x,int16_t y,
 const CameraRegion *region,int16_t locked_y,uint8_t shift,bool *active) {
 if(!active) return;
 bool inside=camera_in_region(x,y,region);
 int16_t normal_y=cam.position.y;
 if(!inside && !*active) return;
 camera_set_y(previous_y);
 camera_approach_y(inside?locked_y:normal_y,shift);
 *active=inside || cameraPosition.y!=normal_y;
}
