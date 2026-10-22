#include <assert.h>
#include "level_progress.h"
int main(void) {
 level_progress_reset();
 assert(!level_progress_mark(LEVEL_PORTAL_HUB));
 assert(!level_progress_mark(LEVEL_FINAL));
 assert(!level_progress_mark(LEVEL_COUNT));
 assert(!level_progress_mark((GameLevel)-1));
 for(unsigned p=0;p<4;p++) {
  assert(level_progress_next(p)==(GameLevel)(p+1));
  assert(level_progress_mark((GameLevel)(p+1)));
  assert(level_progress_next(p)==(GameLevel)(LEVEL_T_EARTH_2+p*2));
  assert(level_progress_mark((GameLevel)(LEVEL_T_EARTH_3+p*2)));
  assert(level_progress_next(p)==(GameLevel)(LEVEL_T_EARTH_2+p*2));
  assert(!level_progress_group(0,p));
  assert(level_progress_mark((GameLevel)(LEVEL_T_EARTH_2+p*2)));
  assert(level_progress_group(0,p));
  if(p<3) assert(level_progress_next(p)==LEVEL_PORTAL_HUB);
 }
 assert(level_progress_realm(0) && !level_progress_realm(1));
 assert(level_progress_bits()==0xFFF);
 for(unsigned p=0;p<4;p++) {
  for(unsigned stage=0;stage<3;stage++) {
   GameLevel next=(GameLevel)(LEVEL_E_EARTH_1+p*3+stage);
   assert(level_progress_next(p)==next);
   assert(level_progress_mark(next));
   assert(level_progress_mark(next)); /* Idempotent. */
   assert(level_progress_stage(1,p,stage));
  }
  assert(level_progress_next(p)==LEVEL_PORTAL_HUB);
 }
 assert(level_progress_bits()==0xFFFFFF && level_progress_realm(1));
 assert(!level_progress_stage(2,0,0) && !level_progress_stage(0,4,0) && !level_progress_stage(0,0,3));
 assert(level_progress_next(4)==LEVEL_PORTAL_HUB);
 level_progress_reset(); assert(level_progress_bits()==0);
 return 0;
}
