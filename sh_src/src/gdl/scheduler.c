#include "gdl/gdl.h"
#include "profile.h"
#include <stddef.h>
#include "mars.h"

typedef struct {
    uint32_t request, completed, scene_generation;
    uint16_t slave_ticks, split_y;
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
    /* Asset uploads bypass cache. Reused level addresses must not retain the
     * previous scene's cached pixels on either processor. */
    SHARED->scene_generation++;
    CacheControl(0x11);
    return true;
}
uint32_t gdl_render_frame(const GdlFrame *frame, GdlRenderTiming *timing) {
    const GdlScene *scene = (const GdlScene *)&SHARED->scene;
    if (!initialized || !timing || !gdl_frame_valid(scene, frame)) return 0;
    uint32_t bytes = offsetof(GdlFrame, sprites) + frame->sprite_count * sizeof(GdlSprite);
    gdl_shared_upload((void *)&storage.frame, frame, bytes);
    /* Priority composition costs more in Water's lower bands. Its measured
     * 128/96 split reduces slave wait; ordinary scenes retain equal halves. */
    uint16_t boundary=frame->background_priority?128:GDL_HEIGHT/2;
    SHARED->split_y=boundary;
    SHARED->request = ++sequence;
    GdlSurface surface = gdl_video_backbuffer();
    uint16_t stamp = profile_timer_read();
    gdl_render_rows(scene, frame, &surface, 0, boundary);
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
    /* Keep the immutable job snapshot off the 4 KiB slave stack. Priority
     * composition has nested scanline scratch buffers and band descriptors. */
    static GdlFrame snapshot;
    uint32_t previous = 0;
    uint32_t generation = 0;
    for (;;) {
        uint32_t request = SHARED->request;
        if (request != previous) {
            if (generation != SHARED->scene_generation) {
                CacheControl(0x11);
                generation = SHARED->scene_generation;
            }
            uint16_t stamp = profile_timer_read();
            uint32_t bytes = offsetof(GdlFrame, sprites) + SHARED->frame.sprite_count * sizeof(GdlSprite);
            const volatile uint8_t *source = (const volatile uint8_t *)&SHARED->frame;
            uint8_t *out = (uint8_t *)&snapshot;
            for (uint32_t i = 0; i < bytes; i++) out[i] = source[i];
            GdlSurface surface = gdl_video_backbuffer();
            gdl_render_rows((const GdlScene *)&SHARED->scene, &snapshot,
                            &surface, SHARED->split_y, GDL_HEIGHT);
            SHARED->slave_ticks = profile_timer_read() - stamp;
            SHARED->completed = request;
            previous = request;
        }
    }
}
