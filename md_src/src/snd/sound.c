/* Minimal XGM host adapted from SGDK src/snd/xgm.c and z80_ctrl.c.
 * Driver: Stephane Dallongeville. Manual frame sync, no SGDK runtime dependency.
 */
#include <stdint.h>
#include <stddef.h>
#include "game_config.h"
#include "snd/sound.h"
extern const uint8_t audio_driver[], audio_driver_end[], music_level[], audio_silence[];
#define ZRAM ((volatile uint8_t *)0xA00000)
#define BUS (*(volatile uint16_t *)0xA11100)
#define RESET (*(volatile uint16_t *)0xA11200)
#define YM_STATUS (*(volatile uint8_t *)0xA04000)
#define YM_PORT ((volatile uint8_t *)0xA04000)
#define PSG_PORT (*(volatile uint8_t *)0xC00011)
bool sound_ready;
static uint16_t tempo;
static void take_bus(void) {
    BUS = 0x100;
    RESET = 0x100;
    while (BUS & 0x100) {}
}
static void release_bus(void) { BUS = 0; }
/* RV stays clear for SH-2 ROM access. The Z80 must use the same fixed
 * $880000 mirror as the 68000, not the unmapped low cartridge window.
 * setBank_BC/DE in z80_mac.i80 serialize bit 23 for this mirror. */
static uint32_t cartridge_address(const uint8_t *p) { return (uint32_t)p; }

/* Caller owns the Z80 bus. Resetting its RAM does not reset the sound chips:
 * old PSG noise and FM voices otherwise survive into the next song. */
static void ym_write(uint16_t bank,uint8_t reg,uint8_t value) {
    while (YM_STATUS & 0x80) {}
    YM_PORT[bank]=reg;
    while (YM_STATUS & 0x80) {}
    YM_PORT[bank+1]=value;
}
static void silence_hardware(void) {
    for (uint16_t channel=0;channel<4;channel++) PSG_PORT=0x9F|(channel<<5);
    for (uint16_t channel=0;channel<6;channel++)
        ym_write(0,0x28,channel<3?channel:channel+1);
    for (uint16_t bank=0;bank<4;bank+=2)
        for (uint16_t op=0;op<4;op++)
            for (uint16_t channel=0;channel<3;channel++)
                ym_write(bank,0x40+op*4+channel,0x7F);
    ym_write(0,0x2A,0x80); /* Center DAC before disabling it. */
    ym_write(0,0x2B,0);
    ym_write(0,0x27,0); /* Disable old timers/channel-3 special mode. */
    ym_write(0,0x22,0); /* Disable the previous song's LFO. */
}

static bool start_song(const uint8_t *music) {
    sound_ready = false;
    tempo = 0;
    take_bus();
    silence_hardware();
    /* The bus request halts the CPU while its code is replaced. Keep reset
     * deasserted here: Z80 RAM access requires that state on Genesis. */
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
    if (!music) return true; /* Logo starts with a silent, ready PCM driver. */
    take_bus();
    uint32_t song = cartridge_address(music);
    for (uint16_t i = 0; i < 63; i++) {
        uint32_t address = ((uint32_t)music[i * 4] << 8)
            | ((uint32_t)music[i * 4 + 1] << 16);
        address = address == 0xFFFF00UL ? silence : address + song + 0x100;
        ZRAM[0x1C04 + i * 4] = address >> 8;
        ZRAM[0x1C05 + i * 4] = address >> 16;
        ZRAM[0x1C06 + i * 4] = music[i * 4 + 2];
        ZRAM[0x1C07 + i * 4] = music[i * 4 + 3];
    }
    uint32_t address = song + 0x104 + ((uint32_t)music[0xFC] << 8)
        + ((uint32_t)music[0xFD] << 16);
    for (uint16_t i = 0; i < 4; i++) ZRAM[0x104 + i] = address >> (i * 8);
    ZRAM[0x113] = 0; /* Pending frames; 0x112 is the driver modification flag. */
    ZRAM[0x100] = (ZRAM[0x100] & 15) | 0x40;
    release_bus();
    return true;
}

bool sound_init(void) {
#if GAME_PORTAL_HUB
    return start_song(NULL);
#else
    return start_song(music_level);
#endif
}
#if GAME_PORTAL_HUB
extern const uint8_t logo_scream[], logo_scream_end[], menu_lightning[], menu_lightning_end[], menu_letter[], menu_letter_end[], intro_lightning[], intro_lightning_end[];
static void play_pcm(const uint8_t *sample,const uint8_t *end,uint16_t channel) {
    if (!sound_ready) return;
    uint32_t address=cartridge_address(sample);
    uint32_t length=end-sample;
    take_bus();
    /* Separate sample slots/channels let title hits overlap the lightning bed. */
    uint16_t slot=63-channel, entry=0x1C00+slot*4, command=0x108+channel*2;
    ZRAM[entry]=address>>8; ZRAM[entry+1]=address>>16;
    ZRAM[entry+2]=length>>8; ZRAM[entry+3]=length>>16;
    ZRAM[command]=15; ZRAM[command+1]=slot;
    ZRAM[0x100]|=1u<<channel;
    release_bus();
}
void sound_logo_scream(void) { play_pcm(logo_scream,logo_scream_end,0); }
void sound_intro_lightning(void) { play_pcm(intro_lightning,intro_lightning_end,0); }
void sound_menu_letter(void) { play_pcm(menu_letter,menu_letter_end,1); }
void sound_menu_lightning(void) { play_pcm(menu_lightning,menu_lightning_end,0); }
extern const uint8_t music_intro[], music_earth[], music_fire[], music_water[], music_air[];
bool sound_play_level(uint16_t level) {
    const uint8_t *music;
    switch (level) {
    case 0: music=music_level; break;
    case 1: music=music_earth; break;
    case 2: music=music_fire; break;
    case 3: music=music_water; break;
    case 4: music=music_air; break;
    case 5: music=music_intro; break;
    default: return false;
    }
    return start_song(music);
}
#endif

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
