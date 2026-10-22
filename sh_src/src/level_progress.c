#include "level_progress.h"
static uint32_t completed;
void level_progress_reset(void) { completed=0; }
uint32_t level_progress_bits(void) { return completed; }
static int bit_for(GameLevel level) {
 if(level>=LEVEL_T_EARTH_1 && level<=LEVEL_T_AIR_1) return (level-1)*3;
 if(level>=LEVEL_T_EARTH_2 && level<=LEVEL_T_AIR_3) {
  unsigned id=level-LEVEL_T_EARTH_2;
  return (id/2)*3+id%2+1;
 }
 if(level>=LEVEL_E_EARTH_1 && level<=LEVEL_E_AIR_3) return 12+level-LEVEL_E_EARTH_1;
 return -1;
}
bool level_progress_mark(GameLevel level) {
 int bit=bit_for(level); if(bit<0) return false;
 completed|=1UL<<bit; return true;
}
bool level_progress_stage(unsigned realm,unsigned portal,unsigned stage) {
 return realm<2 && portal<4 && stage<3 && (completed & (1UL<<(realm*12+portal*3+stage)))!=0;
}
bool level_progress_group(unsigned realm,unsigned portal) {
 return level_progress_stage(realm,portal,0) && level_progress_stage(realm,portal,1) && level_progress_stage(realm,portal,2);
}
bool level_progress_realm(unsigned realm) {
 return realm<2 && ((completed>>(realm*12))&0xFFF)==0xFFF;
}
GameLevel level_progress_next(unsigned portal) {
 if(portal>=4) return LEVEL_PORTAL_HUB;
 unsigned realm=level_progress_realm(0)?1:0;
 for(unsigned stage=0;stage<3;stage++) if(!level_progress_stage(realm,portal,stage)) {
  if(realm) return (GameLevel)(LEVEL_E_EARTH_1+portal*3+stage);
  return stage?(GameLevel)(LEVEL_T_EARTH_2+portal*2+stage-1):(GameLevel)(1+portal);
 }
 return LEVEL_PORTAL_HUB;
}
