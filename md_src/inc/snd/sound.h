#ifndef CHAKAN_SOUND_H
#define CHAKAN_SOUND_H
#include <stdbool.h>
#include <stdint.h>
bool sound_init(void);
void sound_update(void);
void sound_logo_scream(void);
void sound_menu_lightning(void);
void sound_intro_lightning(void);
bool sound_play_level(uint16_t level);
extern bool sound_ready;
#endif
