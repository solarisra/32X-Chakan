#include <assert.h>
#include "camera.h"
int main(void) {
 camera_init(1000,800);
 camera_step(300,200); assert(cameraPosition.x==136 && cameraPosition.y==80);
 camera_set_deadzone(100,90,180,130);
 camera_set_position(0,0); camera_step(300,200);
 assert(cameraPosition.x==120 && cameraPosition.y==70);
 camera_init(1000,800);
 camera_set_lookahead(32,16); camera_set_lookahead_smoothing(0);
 camera_step_velocity(160,112,1,1);
 assert(cameraPosition.x==28 && cameraPosition.y==8);
 camera_step_velocity(160,112,-1,-1);
 assert(cameraPosition.x==0 && cameraPosition.y==0);
 camera_set_lookahead(0,0); camera_step_velocity(160,112,1,1);
 assert(cameraPosition.x==0 && cameraPosition.y==0);
 camera_set_lookahead(32,0); camera_set_lookahead_smoothing(2);
 camera_step_velocity(160,112,1,0); assert(cameraPosition.x==4);
 camera_step_velocity(160,112,1,0); assert(cameraPosition.x==10);
 camera_step_velocity(160,112,0,0); assert(cameraPosition.x==10);
 camera_set_cinematic_center(500,400,0); camera_step(0,0);
 assert(cameraPosition.x==340 && cameraPosition.y==288);
 camera_set_cinematic_center(1000,800,1); camera_step(0,0);
 assert(cameraPosition.x==510 && cameraPosition.y==432);
 camera_set_follow(); camera_set_lookahead(0,0); camera_step(160,112);
 assert(cameraPosition.x==4 && cameraPosition.y==8);
 camera_set_position(500,300); camera_approach_y(301,15); assert(cameraPosition.y==301);
 camera_approach_y(299,1); assert(cameraPosition.y==300);
 camera_approach_y(299,1); assert(cameraPosition.y==299);
 /* Shake never feeds back into the logical position or exceeds map bounds. */
 camera_set_cinematic_center(500,400,0); camera_shake(8,5);
 bool moved=false;
 for(int i=0;i<5;i++) {
  camera_step(0,0);
  moved|=cameraPosition.x!=340 || cameraPosition.y!=288;
 }
 assert(moved); camera_step(0,0);
 assert(cameraPosition.x==340 && cameraPosition.y==288);
 camera_set_cinematic_center(32767,32767,0); camera_shake(255,30);
 for(int i=0;i<40;i++) {
  camera_step(0,0); assert(cameraPosition.x>=0 && cameraPosition.x<=680);
  assert(cameraPosition.y>=0 && cameraPosition.y<=576);
 }
 camera_set_position(500,300);
 GdlFrame f={0}; GdlTilemap background={.width_tiles=64,.height_tiles=62};
 camera_apply_foreground(&f); assert(f.camera_x==500 && f.camera_y==300);
 camera_apply_parallax(&f,&background,1,2); assert(f.background_x==250 && f.background_y==150);
 camera_apply_parallax(&f,&background,-1,1); assert(f.background_x==12 && f.background_y==196);
 camera_apply_parallax(&f,&background,1,0); assert(f.background_y==196);
 camera_apply_bg_b_const_scroll_y(&f,&background,-32); assert(f.background_y==495);
 camera_apply_bg_b_const_scroll_y(&f,&background,32); assert(f.background_y==0);
 camera_apply_bg_b_const_scroll_y(&f,&background,32); assert(f.background_y==0);
 camera_apply_bg_b_const_scroll_y(&f,&background,32); assert(f.background_y==1);
 CameraRegion region={10,20,30,40}; AABB box={{10,30},{52,88}};
 assert(camera_check_bounds_in_region(&box,&region));
 assert(camera_get_region_locked_y(123,&box,&region,580)==580);
 box.min.x=21; assert(camera_get_region_locked_y(123,&box,&region,580)==123);
 camera_init(100,100); camera_step(32767,32767);
 assert(cameraPosition.x==0 && cameraPosition.y==0);
 camera_shake(255,10);
 for(int i=0;i<12;i++) { camera_step(0,0); assert(cameraPosition.x==0 && cameraPosition.y==0); }
 CameraPosition p=camera_get_pos(); assert(p.x==0 && p.y==0);
 return 0;
}
