#include "gdl/gdl.h"
#include "profile.h"
#include <stddef.h>

typedef struct {
    uint32_t request, completed;
    uint16_t slave_ticks;
    GdlScene scene;
    GdlFrame frame;
} GdlShared;
static GdlShared storage;
#define SHARED ((volatile GdlShared *)((uintptr_t)&storage | 0x20000000UL))
static uint32_t sequence;
static bool initialized;

void gdl_shared_upload(void *destination, const void *source, uint32_t bytes) {
    volatile uint8_t *out = (volatile uint8_t *)((uintptr_t)destination | 0x20000000UL);
    const uint8_t *in = source;
    for (uint32_t i = 0; i < bytes; i++) out[i] = in[i];
}
bool gdl_scheduler_init(const GdlScene *scene) {
    if (!gdl_scene_valid(scene) || (initialized && SHARED->request != SHARED->completed)) return false;
    gdl_shared_upload((void *)&storage.scene, scene, sizeof(*scene));
    if (!initialized) {
        SHARED->request = SHARED->completed = 0;
        sequence = 0;
    }
    initialized = true;
    return true;
}
uint32_t gdl_render_frame(const GdlFrame *frame, GdlRenderTiming *timing) {
    const GdlScene *scene = (const GdlScene *)&SHARED->scene;
    if (!initialized || !timing || !gdl_frame_valid(scene, frame)) return 0;
    uint32_t bytes = offsetof(GdlFrame, sprites) + frame->sprite_count * sizeof(GdlSprite);
    gdl_shared_upload((void *)&storage.frame, frame, bytes);
    SHARED->request = ++sequence;
    GdlSurface surface = gdl_video_backbuffer();
    uint16_t stamp = profile_timer_read();
    gdl_render_rows(scene, frame, &surface, 0, GDL_HEIGHT / 2);
    timing->master_draw = profile_timer_read() - stamp;
    stamp = profile_timer_read();
    while (SHARED->completed != sequence) {}
    timing->slave_draw = SHARED->slave_ticks;
    timing->master_wait = profile_timer_read() - stamp;
    stamp = profile_timer_read();
    gdl_video_present();
    timing->flip_wait = profile_timer_read() - stamp;
    return sequence;
}
void gdl_slave_loop(void) {
    uint32_t previous = 0;
    for (;;) {
        uint32_t request = SHARED->request;
        if (request != previous) {
            uint16_t stamp = profile_timer_read();
            GdlFrame snapshot;
            uint32_t bytes = offsetof(GdlFrame, sprites) + SHARED->frame.sprite_count * sizeof(GdlSprite);
            const volatile uint8_t *source = (const volatile uint8_t *)&SHARED->frame;
            uint8_t *out = (uint8_t *)&snapshot;
            for (uint32_t i = 0; i < bytes; i++) out[i] = source[i];
            GdlSurface surface = gdl_video_backbuffer();
            gdl_render_rows((const GdlScene *)&SHARED->scene, &snapshot,
                            &surface, GDL_HEIGHT / 2, GDL_HEIGHT);
            SHARED->slave_ticks = profile_timer_read() - stamp;
            SHARED->completed = request;
            previous = request;
        }
    }
}
