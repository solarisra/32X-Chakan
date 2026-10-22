#include "game_config.h"
.section .rodata
.balign 256
.global music_level
music_level:
#if GAME_PORTAL_HUB
.incbin "md_src/res/music_stage_select.bin"
.balign 256
.global music_intro
music_intro:
.incbin "md_src/res/music_intro.bin"
.balign 256
.global music_earth, music_fire, music_water, music_air
music_earth:
.incbin "md_src/res/music_earth_dimension_1.bin"
.balign 256
music_fire:
.incbin "md_src/res/music_fire_dimension_1.bin"
.balign 256
music_water:
.incbin "md_src/res/music_water_dimension_1.bin"
.balign 256
music_air:
.incbin "md_src/res/music_air_dimension_1.bin"
#elif GAME_T_FIRE_1
.incbin "md_src/res/music_fire_dimension_1.bin"
#elif GAME_T_WATER_1
.incbin "md_src/res/music_water_dimension_1.bin"
#elif GAME_T_AIR_1
.incbin "md_src/res/music_air_dimension_1.bin"
#elif GAME_T_EARTH_1
.incbin "md_src/res/music_earth_dimension_1.bin"
#else
.incbin "md_src/res/music_stage_select.bin"
#endif
#if GAME_PORTAL_HUB
.balign 256
.global intro_lightning, intro_lightning_end
intro_lightning:
.incbin "md_src/res/intro_lightning.bin"
intro_lightning_end:
.balign 256
.global menu_letter, menu_letter_end
menu_letter:
.incbin "md_src/res/menu_letter.bin"
menu_letter_end:
.balign 256
.global menu_lightning, menu_lightning_end
menu_lightning:
.incbin "md_src/res/menu_lightning.bin"
menu_lightning_end:
.balign 256
.global logo_scream, logo_scream_end
logo_scream:
.incbin "md_src/res/logo_scream.bin"
logo_scream_end:
#endif
.balign 256
.global audio_silence
audio_silence:
.space 256
.balign 2
.global audio_driver
.global audio_driver_end
.global audio_rom_end
audio_driver:
.incbin "md_src/res/drv_xgm.bin"
audio_driver_end:
audio_rom_end:
