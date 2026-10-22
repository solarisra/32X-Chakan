#ifndef CHAKAN_LEVEL_PROGRESS_H
#define CHAKAN_LEVEL_PROGRESS_H
#include "level_manager.h"
/* Portal order: Earth, Fire, Water, Air. Realms: terrestrial 0, elemental 1.
 * Stage indices are 0..2. State lasts for the current game; no save storage. */
void level_progress_reset(void);
bool level_progress_mark(GameLevel level);
bool level_progress_stage(unsigned realm,unsigned portal,unsigned stage);
bool level_progress_group(unsigned realm,unsigned portal);
bool level_progress_realm(unsigned realm);
GameLevel level_progress_next(unsigned portal);
uint32_t level_progress_bits(void);
#endif
