#include "level_manager.h"
#include "level_progress.h"
#include "levels/hub/portal_hub.h"
#include "levels/terrestrial/earth_levels.h"
#include "levels/terrestrial/fire/t_fire_1.h"
#include "levels/terrestrial/water/t_water_1.h"
#include "levels/terrestrial/air/t_air_1.h"
#include "input_controls.h"
#include "player.h"

typedef struct {
    void (*init)(void);
    void (*update)(uint16_t);
    const GdlFrame *(*frame)(void);
} LevelRuntime;
static const LevelRuntime levels[LEVEL_T_AIR_1+1]={
    {portal_hub_init,portal_hub_update,portal_hub_render_frame},
    {t_earth1_init,t_earth1_update,t_earth1_render_frame},
    {t_fire1_init,t_fire1_update,t_fire1_render_frame},
    {t_water1_init,t_water1_update,t_water1_render_frame},
    {t_air1_init,t_air1_update,t_air1_render_frame}
};
static GameLevel current, destination;
static uint16_t fade_step;
static bool transitioning;
static bool return_held;

static void begin_transition(GameLevel target) {
    destination=target;
    transitioning=true;
    fade_step=0;
    player_set_control_enabled(false);
    player_set_simulation_enabled(false);
    gdl_palette_begin_fade();
}

bool level_manager_available(GameLevel level) {
    return level>=LEVEL_PORTAL_HUB && level<=LEVEL_T_AIR_1;
}
bool level_manager_complete_current(void) {
    if (transitioning || current==LEVEL_PORTAL_HUB || !level_progress_mark(current)) return false;
    begin_transition(LEVEL_PORTAL_HUB);
    return true;
}
void level_manager_init(void) {
    level_progress_reset();
    current=destination=LEVEL_PORTAL_HUB;
    transitioning=false;
    fade_step=0;
    return_held=false;
    levels[current].init();
    level_music_play(current);
}
void level_manager_update(uint16_t pad) {
    bool return_down=(pad&BUTTON_RETURN_HUB)==BUTTON_RETURN_HUB;
    bool return_pressed=return_down && !return_held;
    return_held=return_down;
    /* Consume the chord before gameplay can interpret A/C as a jump or roll. */
    if (!transitioning && current!=LEVEL_PORTAL_HUB && return_pressed)
        begin_transition(LEVEL_PORTAL_HUB);
    if (transitioning && fade_step==20) {
        current=destination;
        /* gdl_render_frame is synchronous: the old slave job is complete.
         * Each loader fully replaces its overlaid assets before rebinding. */
        levels[current].init();
        prevJoyInput=pad; /* A held Up/jump is not a new press after loading. */
        level_music_play(current);
        transitioning=false;
    }
    levels[current].update(transitioning?0:pad);
    if (transitioning) {
        gdl_palette_fade_to_black(++fade_step,20);
    } else if (current==LEVEL_PORTAL_HUB) {
        GameLevel target=portal_hub_take_destination();
        if (target>LEVEL_PORTAL_HUB && level_manager_available(target)) {
            begin_transition(target);
        }
    }
}
const GdlFrame *level_manager_render_frame(void) { return levels[current].frame(); }
GameLevel level_manager_current(void) { return current; }
