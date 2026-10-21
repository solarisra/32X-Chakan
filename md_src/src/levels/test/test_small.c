#include "test_small.h"
#include "common.h"
#include "player.h"
#include "player_sprite.h"
#include "input_controls.h"
#include "test_small_data.h"

#define VRAM_PLANE_B      0xA000
#define VRAM_SPRITES      0xFC00
#define VRAM_TILES        0x0000
#define PLANE_WIDTH       64
#define PLANE_HEIGHT      32
#define WORLD_HEIGHT      240
#define VRAM_PLANE_A      0x8000

static volatile uint16_t* const vdp_data = (uint16_t*) VDP_DATA_PORT;
static volatile uint16_t* const vdp_ctrl = (uint16_t*) VDP_CTRL_PORT;
static volatile uint32_t* const vdp_ctrl_long = (uint32_t*) VDP_CTRL_PORT;

__attribute__((section(".data")))
static void vdp_set_vram_address(uint16_t address) {
	*vdp_ctrl_long = ((uint32_t)(0x4000 | (address & 0x3FFF)) << 16) | (address >> 14);
}

static void vdp_set_cram_address(uint16_t address) {
	*vdp_ctrl_long = ((uint32_t)(0xC000 | (address & 0x3FFF)) << 16) | (address >> 14);
}

static void vdp_set_register(uint16_t reg, uint16_t value) {
	*vdp_ctrl = 0x8000 | (reg << 8) | value;
}

static void load_words(uint16_t address, const uint16_t *words, uint16_t count) {
    vdp_set_vram_address(address);
    for (uint16_t i = 0; i < count; i++) *vdp_data = words[i];
}

static void draw_plane(uint16_t address, const uint16_t *map,
                       uint16_t width, uint16_t height, uint16_t attributes) {
    vdp_set_vram_address(address);
    for (uint16_t y = 0; y < PLANE_HEIGHT; y++) {
        for (uint16_t x = 0; x < PLANE_WIDTH; x++) {
            *vdp_data = (x < width && y < height)
                ? attributes + map[y * width + x] : 0;
        }
    }
}

static void draw_background(void) {
    load_words(LEVEL_FG_BASE * 32, level_fg_tiles, LEVEL_FG_COUNT * 16);
    load_words(LEVEL_BG_BASE * 32, level_bg_tiles, LEVEL_BG_COUNT * 16);
    draw_plane(VRAM_PLANE_A, level_fg_map, LEVEL_FG_WIDTH, LEVEL_FG_HEIGHT,
               0x8000 | LEVEL_FG_BASE);
    draw_plane(VRAM_PLANE_B, level_bg_map, LEVEL_BG_WIDTH, LEVEL_BG_HEIGHT,
               0x2000 | LEVEL_BG_BASE);
}

static uint16_t star_drift;

/* Temporary axis-separated solid-cell solver, not the original slope system. */
static bool blocked(int16_t x, int16_t y) {
    int16_t left = x / 16;
    int16_t right = (x + PLAYER_WIDTH - 1) / 16;
    int16_t top = y / 16;
    int16_t bottom = (y + PLAYER_HEIGHT - 1) / 16;
    if (x < 0 || x + PLAYER_WIDTH > 320 || y < 0) return 1;
    for (int16_t ty = top; ty <= bottom && ty < 15; ty++) {
        for (int16_t tx = left; tx <= right; tx++) {
            if (level_collision[ty * 20 + tx]) return 1;
        }
    }
    return 0;
}

void test_small_init(void) {
    playerInit(LEVEL_SPAWN_X + PLAYER_OFFSET_X, LEVEL_SPAWN_Y + PLAYER_OFFSET_Y, blocked);
    input_init();
    star_drift = 0;
	/* Keep planes, horizontal scroll, and sprites in distinct VRAM regions. */
	vdp_set_register(2, 0x20); /* Plane A at 0x8000. */
	vdp_set_register(4, 0x05); /* Plane B at 0xA000. */
	vdp_set_register(13, 0x3C); /* H-scroll table at 0xF000. */
	vdp_set_register(16, 1);

	/* Blank the display while uploading; retain V28 (224 visible lines).
     * The bottom 16 pixels of the 240-pixel world are outside the viewport. */
    vdp_set_register(1, 0x34);
    vdp_set_register(5, 0x7E); /* Sprite table at 0xFC00. */
    vdp_set_cram_address(0);
    for (uint16_t i = 0; i < 64; i++) *vdp_data = 0;
    vdp_set_cram_address(0);
    for (uint16_t i = 0; i < 16; i++) *vdp_data = level_fg_palette[i];
    vdp_set_cram_address(0x20);
    for (uint16_t i = 0; i < 16; i++) *vdp_data = level_bg_palette[i];
    vdp_set_vram_address(0);
    for (uint16_t i = 0; i < 16; i++) *vdp_data = 0;
    vdp_set_vram_address(0xF000);
    *vdp_data = 0;
    *vdp_data = 0;
    *vdp_ctrl_long = 0x40000010; /* VSRAM write, address 0. */
    *vdp_data = 0;
    *vdp_data = 0;

	draw_background();
    player_sprite_init();
    vdp_set_register(1, 0x74);
}

__attribute__((section(".data")))
void test_small_update(uint16_t joypad) {
    processInput(joypad);
    updatePlayer();
    if (playerBody.y >= WORLD_HEIGHT) {
        playerInit(LEVEL_SPAWN_X + PLAYER_OFFSET_X, LEVEL_SPAWN_Y + PLAYER_OFFSET_Y, blocked);
        /* Keep previous pad state: holding jump must not become a new press. */
    }
    /* Original star_drift += FIX16(.5), wrapping through the 256px plane. */
    star_drift = (star_drift + 1) & 511;
    *vdp_ctrl_long = 0x40020010; /* VSRAM address 2: plane B only. */
    *vdp_data = star_drift >> 1;
    player_sprite_update();
}
