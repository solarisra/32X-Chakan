#include "mars.h"
#include "game_config.h"
#include "profile.h"
#if GAME_SH2
#if GAME_PORTAL_HUB
#include "level_manager.h"
#include "gamestate.h"
#include "gamestates/gamestate_logo.h"
#define level_init gamestate_init
#define level_update gamestate_update
#define level_render_frame gamestate_render_frame
#elif GAME_T_FIRE_1
#include "levels/terrestrial/fire/t_fire_1.h"
#define level_init t_fire1_init
#define level_update t_fire1_update
#define level_render_frame t_fire1_render_frame
#elif GAME_T_WATER_1
#include "levels/terrestrial/water/t_water_1.h"
#define level_init t_water1_init
#define level_update t_water1_update
#define level_render_frame t_water1_render_frame
#elif GAME_T_AIR_1
#include "levels/terrestrial/air/t_air_1.h"
#define level_init t_air1_init
#define level_update t_air1_update
#define level_render_frame t_air1_render_frame
#elif GAME_T_EARTH_1
#include "levels/terrestrial/earth_levels.h"
#define level_init t_earth1_init
#define level_update t_earth1_update
#define level_render_frame t_earth1_render_frame
#else
#include "levels/test/test_wide.h"
#define level_init test_wide_init
#define level_update test_wide_update
#define level_render_frame test_wide_render_frame
#endif
#include "player.h"
#include "camera.h"
#endif

#if GAME_PORTAL_HUB
void gamestate_changed(GAME_STATE state) {
    while (MARS_SYS_COMM0) {}
    MARS_SYS_COMM0=0x0C00 | state;
    while (MARS_SYS_COMM0) {}
}
void gamestate_intro_lightning(void) {
    while (MARS_SYS_COMM0) {}
    MARS_SYS_COMM0=0x0E00;
    while (MARS_SYS_COMM0) {}
}
void gamestate_menu_letter(void) {
    while (MARS_SYS_COMM0) {}
    MARS_SYS_COMM0=0x0D00;
    while (MARS_SYS_COMM0) {}
}
void gamestate_logo_scream(void) {
    while (MARS_SYS_COMM0) {}
    MARS_SYS_COMM0=0x0B00;
    while (MARS_SYS_COMM0) {}
}
void level_music_play(GameLevel level) {
    while (MARS_SYS_COMM0) {}
    MARS_SYS_COMM0=0x0A00 | level;
    while (MARS_SYS_COMM0) {}
}
#endif

int m_main(void) {
#if GAME_SH2
    gdl_video_init();
    while (MARS_SYS_COMM0) {}
    MARS_SYS_COMM0 = 0x0900;
    while (MARS_SYS_COMM0) {}
    level_init();
    profile_timer_init();
    uint32_t totals[5] = {0};
    uint16_t maxima[5] = {0};
    uint32_t samples = 0;
    uint32_t last_tick = MARS_SYS_COMM12;
    for (;;) {
        uint32_t tick;
        do { tick = MARS_SYS_COMM12; } while (tick == last_tick);
        uint32_t elapsed = tick - last_tick;
        last_tick = tick;
        if (elapsed > 4) elapsed = 4; /* Bound catch-up after reset/debug pauses. */
        uint16_t duration[5];
        uint16_t stamp = profile_timer_read();
        uint16_t pad = MARS_SYS_COMM8;
        for (uint32_t i = 0; i < elapsed; i++) level_update(pad);
        duration[0] = profile_timer_read() - stamp;
        MARS_SYS_COMM4 = cameraPosition.x;
        MARS_SYS_COMM6 = playerBody.x;
        GdlRenderTiming timing;
        uint32_t sequence = gdl_render_frame(level_render_frame(), &timing);
        if (!sequence) continue;
#if GAME_PORTAL_HUB
        gamestate_frame_presented();
#endif
        duration[1] = timing.master_draw;
        duration[2] = timing.slave_draw;
        duration[3] = timing.master_wait;
        duration[4] = timing.flip_wait;
        MARS_SYS_COMM2 = sequence;
        samples++;
        for (uint16_t i = 0; i < 5; i++) {
            totals[i] += duration[i];
            if (duration[i] > maxima[i]) maxima[i] = duration[i];
        }
        /* 68000 copies profiling counters to work RAM for the headless harness. */
        for (uint16_t i = 0; i < 10; i++) {
            while (MARS_SYS_COMM0) {}
            uint32_t value = i < 5 ? totals[i] / samples : maxima[i - 5];
            if (value > 1023) value = 1023;
            MARS_SYS_COMM0 = 0x8000 | (i << 10) | value;
            while (MARS_SYS_COMM0) {}
        }
    }
#else
    Hw32xInit(MARS_VDP_MODE_256, 0);
    Hw32xSetBGColor(0, 0, 0, 0);
    for (;;) {}
#endif
}
