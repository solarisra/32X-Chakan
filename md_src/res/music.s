.section .rodata
.balign 256
.global music_stage_select
music_stage_select:
.incbin "md_src/res/music_stage_select.bin"
.balign 256
.global audio_silence
audio_silence:
.space 256
.balign 2
.global audio_driver
.global audio_driver_end
audio_driver:
.incbin "md_src/res/drv_xgm.bin"
audio_driver_end:
