#ifndef CHAKAN_SH_LEVEL_MANAGER_H
#define CHAKAN_SH_LEVEL_MANAGER_H
#include "gdl/gdl.h"
typedef enum {
    LEVEL_PORTAL_HUB, LEVEL_T_EARTH_1, LEVEL_T_FIRE_1,
    LEVEL_T_WATER_1, LEVEL_T_AIR_1,
    LEVEL_T_EARTH_2, LEVEL_T_EARTH_3, LEVEL_T_FIRE_2, LEVEL_T_FIRE_3,
    LEVEL_T_WATER_2, LEVEL_T_WATER_3, LEVEL_T_AIR_2, LEVEL_T_AIR_3,
    LEVEL_E_EARTH_1, LEVEL_E_EARTH_2, LEVEL_E_EARTH_3,
    LEVEL_E_FIRE_1, LEVEL_E_FIRE_2, LEVEL_E_FIRE_3,
    LEVEL_E_WATER_1, LEVEL_E_WATER_2, LEVEL_E_WATER_3,
    LEVEL_E_AIR_1, LEVEL_E_AIR_2, LEVEL_E_AIR_3,
    LEVEL_FINAL, LEVEL_COUNT
} GameLevel;
/* Called with the renderer idle. One active level owns the shared asset RAM. */
void level_manager_init(void);
void level_manager_update(uint16_t pad);
const GdlFrame *level_manager_render_frame(void);
GameLevel level_manager_current(void);
bool level_manager_available(GameLevel level);
/* Called by a stage exit: record completion and fade back to the Hub. */
bool level_manager_complete_current(void);
/* Backend supplied by the master: sends the selected track to the 68000. */
void level_music_play(GameLevel level);
#endif
