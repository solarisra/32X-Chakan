/* Minimal XGM host adapted from SGDK src/snd/xgm.c and z80_ctrl.c.
 * Driver: Stephane Dallongeville. Manual frame sync, no SGDK runtime dependency.
 */
#include <stdint.h>
#include "snd/sound.h"
extern const uint8_t audio_driver[], audio_driver_end[], music_stage_select[], audio_silence[];
#define ZRAM ((volatile uint8_t *)0xA00000)
#define BUS (*(volatile uint16_t *)0xA11100)
#define RESET (*(volatile uint16_t *)0xA11200)
bool sound_ready;
static uint16_t tempo;
static void take_bus(void) {
    BUS = 0x100;
    RESET = 0x100;
    while (BUS & 0x100) {}
}
static void release_bus(void) { BUS = 0; }
/* 68000 code is linked at the 32X mirror; Z80 uses the cartridge ROM window. */
static uint32_t cartridge_address(const uint8_t *p) { return (uint32_t)p - 0x880000UL; }

bool sound_init(void) {
    sound_ready = false;
    tempo = 0;
    take_bus();
    for (uint16_t i = 0; i < 8192; i++) ZRAM[i] = 0;
    for (uint16_t i = 0; i < (uint16_t)(audio_driver_end - audio_driver); i++) ZRAM[i] = audio_driver[i];
    uint32_t silence = cartridge_address(audio_silence);
    ZRAM[0x1C00] = silence >> 8;
    ZRAM[0x1C01] = silence >> 16;
    ZRAM[0x1C02] = 1;
    ZRAM[0x1C03] = 0;
    RESET = 0;
    release_bus();
    for (volatile uint16_t delay = 0; delay < 1000; delay++) {}
    RESET = 0x100;
    /* Bounded readiness polling: a failed driver cannot freeze gameplay. */
    for (uint16_t attempts = 0; attempts < 10000; attempts++) {
        take_bus();
        bool ready = (ZRAM[0x102] & 0x80) != 0;
        release_bus();
        if (ready) { sound_ready = true; break; }
        for (volatile uint16_t delay = 0; delay < 100; delay++) {}
    }
    if (!sound_ready) return false;
    take_bus();
    uint32_t song = cartridge_address(music_stage_select);
    for (uint16_t i = 0; i < 63; i++) {
        uint32_t address = ((uint32_t)music_stage_select[i * 4] << 8)
            | ((uint32_t)music_stage_select[i * 4 + 1] << 16);
        address = address == 0xFFFF00UL ? silence : address + song + 0x100;
        ZRAM[0x1C04 + i * 4] = address >> 8;
        ZRAM[0x1C05 + i * 4] = address >> 16;
        ZRAM[0x1C06 + i * 4] = music_stage_select[i * 4 + 2];
        ZRAM[0x1C07 + i * 4] = music_stage_select[i * 4 + 3];
    }
    uint32_t address = song + 0x104 + ((uint32_t)music_stage_select[0xFC] << 8)
        + ((uint32_t)music_stage_select[0xFD] << 16);
    for (uint16_t i = 0; i < 4; i++) ZRAM[0x104 + i] = address >> (i * 8);
    ZRAM[0x113] = 0; /* Pending frames; 0x112 is the driver modification flag. */
    ZRAM[0x100] = (ZRAM[0x100] & 15) | 0x40;
    release_bus();
    return true;
}

void sound_update(void) {
    if (!sound_ready) return;
    /* XGM was converted to 60Hz; PAL consumes six song ticks per five frames. */
    tempo += 60;
    uint16_t hz = (*(volatile uint16_t *)0xC00004 & 1) ? 50 : 60;
    uint16_t frames = tempo / hz;
    tempo %= hz;
    for (uint16_t attempts = 0; attempts < 100; attempts++) {
        take_bus();
        if (!ZRAM[0x112]) {
            ZRAM[0x113] += frames;
            release_bus();
            return;
        }
        release_bus();
        for (volatile uint16_t delay = 0; delay < 20; delay++) {}
    }
}
