#ifndef GDL_LEVEL_ASSETS_H
#define GDL_LEVEL_ASSETS_H
/* mars.ld overlays these fully reloaded arrays. Switch only after both SH-2s
 * finish the previous frame; keep persistent state outside these sections. */
#define GDL_LEVEL_ASSETS(level) __attribute__((section(".bss.level_" #level)))
#endif
