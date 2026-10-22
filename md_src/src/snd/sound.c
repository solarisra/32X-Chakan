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
static bool music_paused;
#if GAME_PORTAL_HUB
/* Chip registers are write-only. Follow the XGM stream to retain their state. */
typedef struct {
    const uint8_t *base, *next;
    uint32_t ticks;
    uint8_t ym[2][256], keys[8], latch;
    uint16_t psg[8];
} MusicState;
static MusicState registers, saved_registers;
static uint8_t saved_driver[8192];
static uint16_t saved_tempo;
static bool context_saved;

static void follow_music(uint32_t target) {
    while (registers.next && registers.ticks < target) {
        /* xgmtool -s emits a length-prefixed frame with doubled opcodes. */
        const uint8_t *end=registers.next+*registers.next;
        registers.next++;
        bool extra=false;
        while (registers.next && registers.next<end) {
            uint8_t command=*registers.next++ >> 1;
            unsigned count=(command&15)+1;
            switch (command>>4) {
            case 1:
                if (command>=0x1C) break;
                count=(command<0x18 ? command&7 : command&3)+1;
                while (count--) {
                    uint8_t value=*registers.next++;
                    if (value&0x80) {
                        registers.latch=(value>>4)&7;
                        unsigned r=registers.latch;
                        registers.psg[r]=(registers.psg[r]&0x3F0)|(value&15);
                    } else {
                        unsigned r=registers.latch;
                        if (!(r&1) && r<6)
                            registers.psg[r]=(registers.psg[r]&15)|((value&63)<<4);
                        else registers.psg[r]=value&15;
                    }
                }
                break;
            case 2: case 3:
                while (count--) {
                    uint8_t reg=*registers.next++;
                    registers.ym[(command>>4)-2][reg]=*registers.next++;
                }
                break;
            case 4:
                if (command>0x45) break;
                while (count--) {
                    uint8_t value=*registers.next++;
                    registers.keys[value&7]=value;
                }
                break;
            case 5: registers.next++; break;
            case 6: case 7:
                if (command<0x70) registers.next+=count*2; /* Driver's RR/PSG save state. */
                else if (command==0x7D) extra=true; /* Additional stream frame this tick. */
                else if (command==0x7E) {
                    const uint8_t *p=registers.next;
                    registers.next=registers.base+p[0]+((uint32_t)p[1]<<8)+((uint32_t)p[2]<<16);
                    end=registers.next;
                } else if (command==0x7F) registers.next=NULL;
                break;
            }
        }
        if (!extra) registers.ticks++;
    }
}
#endif
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
    music_paused = false;
#if GAME_PORTAL_HUB
    registers=(MusicState){0};
    for (unsigned i=1;i<8;i+=2) registers.psg[i]=15;
    for (unsigned i=0;i<8;i++) registers.keys[i]=i;
#endif
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
#if GAME_PORTAL_HUB
    registers.base=registers.next=(const uint8_t *)address;
#endif
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
extern const uint8_t logo_scream[], logo_scream_end[], menu_lightning[], menu_lightning_end[], intro_lightning[], intro_lightning_end[];
static void play_pcm(const uint8_t *sample,const uint8_t *end,uint16_t channel) {
    if (!sound_ready) return;
    uint32_t address=cartridge_address(sample);
    uint32_t length=end-sample;
    take_bus();
    /* Keep each effect channel in its own sample slot. */
    uint16_t slot=63-channel, entry=0x1C00+slot*4, command=0x108+channel*2;
    ZRAM[entry]=address>>8; ZRAM[entry+1]=address>>16;
    ZRAM[entry+2]=length>>8; ZRAM[entry+3]=length>>16;
    ZRAM[command]=15; ZRAM[command+1]=slot;
    ZRAM[0x100]|=1u<<channel;
    release_bus();
}
void sound_logo_scream(void) { play_pcm(logo_scream,logo_scream_end,0); }
void sound_intro_lightning(void) { play_pcm(intro_lightning,intro_lightning_end,0); }
void sound_menu_lightning(void) { play_pcm(menu_lightning,menu_lightning_end,2); }
extern const uint8_t music_pause[], music_intro[], music_earth[], music_fire[], music_water[], music_air[];
bool sound_play_level(uint16_t level) {
    const uint8_t *music;
    switch (level) {
    case 0: music=music_level; break;
    case 1: music=music_earth; break;
    case 2: music=music_fire; break;
    case 3: music=music_water; break;
    case 4: music=music_air; break;
    case 5: music=music_intro; break;
    case 6: music=music_pause; break;
    default: return false;
    }
    return start_song(music);
}
#endif

#if GAME_PORTAL_HUB
static void restore_chips(void) {
    /* Restore operators before frequencies (high byte first), then key gates. */
    for (unsigned bank=0;bank<2;bank++) {
        for (unsigned reg=0x30;reg<0xA0;reg++)
            ym_write(bank*2,reg,registers.ym[bank][reg]);
        for (unsigned reg=0xA4;reg<=0xAE;reg++)
            if ((reg&3)!=3 && (reg<0xA8 || reg>=0xAC)) {
                ym_write(bank*2,reg,registers.ym[bank][reg]);
                ym_write(bank*2,reg-4,registers.ym[bank][reg-4]);
            }
        for (unsigned reg=0xB0;reg<=0xB6;reg++)
            ym_write(bank*2,reg,registers.ym[bank][reg]);
    }
    ym_write(0,0x22,registers.ym[0][0x22]);
    ym_write(0,0x24,registers.ym[0][0x24]);
    ym_write(0,0x25,registers.ym[0][0x25]);
    ym_write(0,0x26,registers.ym[0][0x26]);
    ym_write(0,0x27,registers.ym[0][0x27]);
    ym_write(0,0x2B,registers.ym[0][0x2B]);
    for (unsigned channel=0;channel<8;channel++)
        if ((channel&3)!=3) ym_write(0,0x28,registers.keys[channel]);
    for (unsigned reg=0;reg<8;reg++) {
        PSG_PORT=0x80|(reg<<4)|(registers.psg[reg]&15);
        if (!(reg&1) && reg<6) PSG_PORT=(registers.psg[reg]>>4)&63;
    }
    /* Restore the PSG latch so an unlatch/data write keeps its meaning. */
    PSG_PORT=0x80|(registers.latch<<4)|(registers.psg[registers.latch]&15);
}
#endif

void sound_set_paused(bool paused) {
#if GAME_PORTAL_HUB
    if (paused == music_paused || (!sound_ready && !context_saved)) return;
    if (paused) {
        take_bus();
        ZRAM[0x100]|=0x80; /* Save CPU registers and yield at a frame boundary. */
        release_bus();
        sound_update(); /* Let the frame-end command handler run. */
        bool saved=false;
        for (unsigned attempts=0;attempts<10000;attempts++) {
            take_bus();
            if (ZRAM[0x101]) {
                for (unsigned i=0;i<8192;i++) saved_driver[i]=ZRAM[i];
                saved=true;
            }
            release_bus();
            if (saved) break;
            for (volatile unsigned delay=0;delay<100;delay++) {}
        }
        if (!saved) {
            take_bus(); ZRAM[0x100]&=0x7F; release_bus();
            return;
        }
        uint32_t elapsed=saved_driver[0x194]|((uint32_t)saved_driver[0x195]<<8)
                         |((uint32_t)saved_driver[0x196]<<16);
        follow_music(elapsed);
        saved_registers=registers;
        saved_tempo=tempo;
        context_saved=true;
        start_song(music_pause);
        music_paused=true;
    } else if (context_saved) {
        take_bus();
        silence_hardware();
        for (unsigned i=0;i<8192;i++) ZRAM[i]=saved_driver[i];
        registers=saved_registers;
        /* Reset into the restore entry, which reloads both Z80 register banks. */
        ZRAM[0]=0xC3; ZRAM[1]=0x20; ZRAM[2]=0;
        RESET=0;
        release_bus();
        for (volatile unsigned delay=0;delay<1000;delay++) {}
        /* Z80 reset also resets the YM2612. Restore chips after releasing
         * reset, with the CPU held on the bus until restoration is complete. */
        take_bus();
        restore_chips();
        release_bus();
        tempo=saved_tempo;
        music_paused=false;
        context_saved=false;
        sound_ready=true;
    }
#else
    (void)paused;
#endif
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
            uint32_t elapsed=ZRAM[0x194]|((uint32_t)ZRAM[0x195]<<8)|((uint32_t)ZRAM[0x196]<<16);
            ZRAM[0x113] += frames;
            release_bus();
#if GAME_PORTAL_HUB
            follow_music(elapsed);
#else
            (void)elapsed;
#endif
            return;
        }
        release_bus();
        for (volatile uint16_t delay = 0; delay < 20; delay++) {}
    }
}
