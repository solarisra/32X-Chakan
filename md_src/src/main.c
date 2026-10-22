#include "common.h"
#include "game_config.h"
#include "snd/sound.h"
#include "levels/test/test_small.h"

// 32X COMM
static volatile uint16_t* const mars_comm0  = (uint16_t*) MARS_COMM0;
static volatile uint16_t* const mars_comm2  = (uint16_t*) MARS_COMM2;
static volatile uint16_t* const mars_comm8  = (uint16_t*) MARS_COMM8;
static volatile uint32_t* const mars_comm12 = (uint32_t*) MARS_COMM12;

// VDP
static volatile uint16_t* const vdp_data_port = (uint16_t*) VDP_DATA_PORT;
static volatile uint16_t* const vdp_ctrl_port = (uint16_t*) VDP_CTRL_PORT;
static volatile uint32_t* const vdp_ctrl_wide = (uint32_t*) VDP_CTRL_PORT;

// External functions
extern uint16_t read_joypad(uint8_t player);

uint32_t timer = 0;
volatile uint16_t sh2_rendered_frames = 0;
volatile uint16_t sh2_camera_x = 0, sh2_player_x = 0;
volatile uint16_t sh2_profile[10];
volatile uint16_t sh2_level, sh2_gamestate;
#if GAME_SH2
static uint16_t raster_clock;
static uint8_t raster_last;
#endif
uint16_t vramOffset = 0;

// It is recommended to put functions that run 1+ times every frame into RAM
// by specifying this attribute before the signature. This keeps the M68K off
// the ROM so the SH-2s can access it without slowdown.
// It should be safe to add or remove it from any function and experiment with
// the speed vs space differences

__attribute__((section(".data")))
void vdp_color(uint16_t index, uint16_t color) {
	index <<= 1;
	*vdp_ctrl_wide = ((0xC000 + (((uint32_t)index) & 0x3FFF)) << 16) + (((uint32_t)index) >> 14);
	*vdp_data_port = color;
}

__attribute__((section(".data")))
void do_commands(void) {
#if GAME_SH2
    uint8_t line = *(volatile uint16_t *)VDP_COUNTER >> 8;
    uint8_t delta = line - raster_last;
    /* NTSC V28 repeats six V-counter values at the end of the frame. */
    if (delta > 128) delta = 1;
    raster_clock += delta;
    raster_last = line;
    *(volatile uint16_t *)MARS_COMM10 = raster_clock;
#endif
	uint16_t cmd = *mars_comm0;
    if (cmd & 0x8000) {
        uint16_t index = (cmd >> 10) & 15;
        if (index < 10) sh2_profile[index] = cmd & 1023;
        *mars_comm0 = 0;
        return;
    }
	switch(cmd >> 8) {
	default: break; // Unknown command
	case 0: return; // No command
	case 3:
		*mars_comm8 = read_joypad(cmd);
		break;
#if GAME_PORTAL_HUB
    case 11: sound_logo_scream(); break;
    case 14: sound_intro_lightning(); break;
    case 15: sound_set_paused((cmd & 1) != 0); break;
    case 12: sh2_gamestate=cmd & 255;
        if (sh2_gamestate==1) sound_play_level(5);
        if (sh2_gamestate==2) { sound_play_level(5); sound_menu_lightning(); }
        break;
    case 10: /* Runtime level/music change, acknowledged after driver readiness. */
        if (sound_play_level(cmd & 255)) sh2_level=cmd & 255;
        break;
#endif
	case 9: /* SH-2 service-ready handshake. */
        break;
	case 4:
		break;
	case 5: // Set VRAM or Plane offset
		vramOffset = *mars_comm2;
		break;
	case 6: // Write tile to Plane B
		*vdp_ctrl_wide = (((uint32_t)0x6000 + ((vramOffset) & 0x3FFF)) << 16) + (((vramOffset) >> 14) | 0x03);
		*vdp_data_port = *mars_comm2;
		vramOffset += 2;
		break;
	case 7: // Write word to VRAM address
		*vdp_ctrl_wide = (((uint32_t)0x4000 + ((vramOffset) & 0x3FFF)) << 16) + (((vramOffset) >> 14) | 0x00);
		*vdp_data_port = *mars_comm2;
		vramOffset += 2;
		break;
	}
	*mars_comm0 = 0;
}

__attribute__((section(".data")))
void main(void) {
#if GAME_SH2
    /* Clear Genesis graphics; the 32X now owns the complete game image. */
    *vdp_ctrl_port = 0x8134;
    *vdp_ctrl_wide = 0x40000000;
    for (uint32_t i = 0; i < 32768; i++) *vdp_data_port = 0;
    *vdp_ctrl_wide = 0xC0000000;
    for (uint16_t i = 0; i < 64; i++) *vdp_data_port = 0;
    *vdp_ctrl_port = 0x8174;
#else
    test_small_init();
#endif
    sound_init();
#if GAME_SH2
    raster_last = *(volatile uint16_t *)VDP_COUNTER >> 8;
#endif
	while(1) {
		while(*vdp_ctrl_port & 8) do_commands();
		while(!(*vdp_ctrl_port & 8)) do_commands();

        sound_update();
#if GAME_SH2
        *mars_comm8 = read_joypad(0);
        sh2_rendered_frames = *(volatile uint16_t *)MARS_COMM2;
        sh2_camera_x = *(volatile uint16_t *)MARS_COMM4;
        sh2_player_x = *(volatile uint16_t *)MARS_COMM6;
#else
        test_small_update(read_joypad(0));
#endif
		do_commands();
		*mars_comm12 = ++timer;
	}
}
