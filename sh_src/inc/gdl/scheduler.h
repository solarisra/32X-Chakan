#ifndef GDL_SCHEDULER_H
#define GDL_SCHEDULER_H
#include "renderer.h"
typedef struct {
    uint16_t master_draw, slave_draw, master_wait, flip_wait;
} GdlRenderTiming;
/* Upload to caller-owned SDRAM through its uncached alias before binding.
 * Sources can be ROM or SDRAM. Assets must remain immutable while bound. */
void gdl_shared_upload(void *destination, const void *source, uint32_t bytes);
/* Bind on master before submission, with the previous render complete. Rebinding
 * invalidates both CPUs' asset caches. Assets stay immutable until the next bind.
 * Slave can wait before the first init. */
bool gdl_scheduler_init(const GdlScene *scene);
/* Synchronous on master: snapshot, parallel draw, completion barrier, then flip.
 * Returns completed sequence, or zero for invalid input (no swap). */
uint32_t gdl_render_frame(const GdlFrame *frame, GdlRenderTiming *timing);
/* Slave SH-2 entry, never returns. */
void gdl_slave_loop(void);
#endif
