#include "pause_menu.h"
#include "level_manager.h"
#include "input_controls.h"
#include "pause_data.h"
static uint16_t previous;
static const GdlScreen screen={pause_screen,pause_patches,0,pause_rows,0,0,0,0};
void pause_menu_open(uint16_t pad) {
 previous=pad;
 gdl_palette_schedule(80,pause_palette,16);
 gamestate_play_menu_changed(true);
}
void pause_menu_close(uint16_t pad) {
 level_manager_sync_input(pad);
 gamestate_play_menu_changed(false);
}
bool pause_menu_update(uint16_t pad) {
 uint16_t pressed=pad&~previous;previous=pad;
 return (pressed&BUTTON_START)!=0;
}
void pause_menu_frame(GdlFrame *scratch) {
 *scratch=(GdlFrame){0};scratch->screen=&screen;
}
