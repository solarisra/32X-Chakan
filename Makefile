# Sample Makefile for Marsdev (32X version)
# For 32X both m68k and SH-2 code has to be built
# The usual variables are split into "MD" and "SH" versions

# Default paths, can be overridden by setting MARSDEV before calling make
MARSDEV ?= ${HOME}/mars
TOOLSBIN = $(MARSDEV)/bin
MDBIN    = $(MARSDEV)/m68k-elf/bin
SHBIN    = $(MARSDEV)/sh-elf/bin

TARGET  ?= Chakan
PYTHON ?= python3
GAME ?= PORTAL_HUB
ifeq ($(filter $(GAME),T_FIRE_1 T_WATER_1 T_AIR_1 T_EARTH_1 PORTAL_HUB TEST_WIDE TEST_SMALL),)
$(error GAME must be T_FIRE_1, T_WATER_1, T_AIR_1, T_EARTH_1, PORTAL_HUB, TEST_WIDE or TEST_SMALL)
endif
# Update only on a backend change, before make reads compiler dependencies.
CONFIGURE_GAME := $(shell $(PYTHON) sh_src/tools/configure_game.py $(GAME))
MDTARGET = md_start

# m68k GCC and Binutils
MDCC   = $(MDBIN)/m68k-elf-gcc
MDCXX  = $(MDBIN)/m68k-elf-g++
MDAS   = $(MDBIN)/m68k-elf-as
MDLD   = $(MDBIN)/m68k-elf-ld
MDNM   = $(MDBIN)/m68k-elf-nm
MDOBJC = $(MDBIN)/m68k-elf-objcopy
# sh2 GCC and Binutils
SHCC   = $(SHBIN)/sh-elf-gcc
SHCXX  = $(SHBIN)/sh-elf-g++
SHAS   = $(SHBIN)/sh-elf-as
SHLD   = $(SHBIN)/sh-elf-ld
SHNM   = $(SHBIN)/sh-elf-nm
SHOBJC = $(SHBIN)/sh-elf-objcopy

# Some files needed are in a versioned directory
MDCC_VER := $(shell $(MDCC) -dumpversion)
SHCC_VER := $(shell $(SHCC) -dumpversion)

# Need the LTO plugin so NM can dump our symbol table
MDPLUGIN = $(MARSDEV)/m68k-elf/libexec/gcc/m68k-elf/$(MDCC_VER)/liblto_plugin.so
SHPLUGIN = $(MARSDEV)/sh-elf/libexec/gcc/sh-elf/$(SHCC_VER)/liblto_plugin.so

MDINCS   = -Imd_src/inc -Imd_src/res -Imd_src/src -Ibuild
MDINCS  += -I$(MARSDEV)/m68k-elf/lib/gcc/m68k-elf/$(MDCC_VER)/include
SHINCS   = -Ish_src -Ish_src/inc -Ish_src/res -Ish_src/src -Ibuild
SHINCS  += -I$(MARSDEV)/sh-elf/lib/gcc/sh-elf/$(SHCC_VER)/include

MDLIBS   = -L$(MARSDEV)/m68k-elf/lib/gcc/m68k-elf/$(MDCC_VER) -lgcc
SHLIBS   = -L$(MARSDEV)/sh-elf/lib/gcc/sh-elf/$(SHCC_VER) -lgcc

# Any C or C++ standard should be fine here as long as GCC supports it
MDCCFLAGS  = -m68000 -mshort -Wall -Wextra -pedantic -std=c99 -ffreestanding
MDCXXFLAGS = -m68000 -mshort -Wall -Wextra -pedantic -std=c++17 -ffreestanding
SHCCFLAGS  = -m2 -mb -Wall -Wextra -pedantic -std=c99 -ffreestanding
SHCXXFLAGS = -m2 -mb -Wall -Wextra -pedantic -std=c++17 -ffreestanding

# Assembler flags
MDASFLAGS  = -x assembler-with-cpp -Imd_src/inc -Ibuild -m68000 -Wa,--register-prefix-optional
SHASFLAGS  = -Ish_src --small

# Linker flags
MDLDFLAGS  = -T md_src/md.ld -nostdlib
SHLDFLAGS  = -T sh_src/mars.ld -nostdlib

# Extra options set by debug or release target
MDEXTRA = 
SHEXTRA = 

# Generate m68k object target list
MDSS    = $(wildcard md_src/*.s md_src/res/*.s)
MDCS    = $(shell find md_src/src -type f -name '*.c' | sort)
MDCPPS  = $(shell find md_src/src -type f -name '*.cpp' | sort)
MDOBJS  = $(MDSS:.s=.o)
MDOBJS += $(MDCS:.c=.o)
MDOBJS += $(MDCPPS:.cpp=.o)

# Generate sh object target list
SHSS    = $(wildcard sh_src/*.s)
SHCS    = $(wildcard sh_src/*.c) $(shell find sh_src/src -type f -name '*.c' | sort)
SHCPPS  = $(wildcard sh_src/*.cpp)
SHOBJS  = $(SHSS:.s=.o)
SHOBJS += $(SHCS:.c=.o)
SHOBJS += $(SHCPPS:.cpp=.o)

.PHONY: all release debug

all: release

# Track headers as well as source files for both CPUs.
-include $(MDCS:.c=.d) $(SHCS:.c=.d)

md_src/res/test_small_data.h: md_src/tools/convert_test_level.py md_src/res/levels/Hub/fg_test_level.png md_src/res/levels/Hub/portal_starfield.png md_src/src/levels/test/test_small_collision.json
	$(PYTHON) md_src/tools/convert_test_level.py

md_src/src/levels/test/test_small.o: md_src/res/test_small_data.h

md_src/res/sprites.h: md_src/tools/convert_sprites.py md_src/tools/convert_test_level.py md_src/res/sprites/Chakan/chakan_anim_no_weapons.png
	$(PYTHON) md_src/tools/convert_sprites.py

md_src/src/player_sprite.o: md_src/res/sprites.h

md_src/res/music_stage_select.bin: md_src/res/music/02\ -\ Stage\ Select.vgm
	$(MDBIN)/xgmtool "$<" $@ -s -n

md_src/res/drv_xgm.bin: md_src/src/snd/drv_xgm.s80 md_src/inc/snd/z80_def.i80 md_src/inc/snd/z80_mac.i80 md_src/inc/snd/z80_fct.i80
	$(MDBIN)/sjasm -imd_src/inc/snd $< $@ md_src/res/drv_xgm.lst

md_src/res/music_earth_dimension_1.bin: md_src/res/music/07\ -\ Earth\ Dimension\ 1.vgm
	$(MDBIN)/xgmtool "$<" $@ -s -n

md_src/res/music_air_dimension_1.bin: md_src/res/music/11\ -\ Air\ Dimension\ 1.vgm
	$(MDBIN)/xgmtool "$<" $@ -s -n

md_src/res/music_water_dimension_1.bin: md_src/res/music/09\ -\ Water\ Dimension\ 1.vgm
	$(MDBIN)/xgmtool "$<" $@ -s -n

md_src/res/music_fire_dimension_1.bin: md_src/res/music/05\ -\ Fire\ Dimension\ 1.vgm
	$(MDBIN)/xgmtool "$<" $@ -s -n

md_src/res/logo_scream.bin: md_src/tools/convert_logo_sound.py md_src/res/sfx/chakan_scream2.wav
	$(PYTHON) md_src/tools/convert_logo_sound.py

md_src/res/music.o: md_src/res/logo_scream.bin md_src/res/music_fire_dimension_1.bin md_src/res/music_water_dimension_1.bin md_src/res/music_air_dimension_1.bin md_src/res/music_stage_select.bin md_src/res/music_earth_dimension_1.bin md_src/res/drv_xgm.bin build/game_config.h


release: MDEXTRA  = -O2 -fomit-frame-pointer -flto -fuse-linker-plugin
release: SHEXTRA  = -Ofast -fomit-frame-pointer -flto -fuse-linker-plugin
release: $(MDTARGET).bin $(MDTARGET).lst $(TARGET).32x $(TARGET).lst

# Gens-KMod, BlastEm and UMDK support GDB tracing, enabled by this target
debug: MDEXTRA = -g -Og -DDEBUG -DKDEBUG
debug: SHEXTRA = -g -Og -DDEBUG -DKDEBUG
debug: $(MDTARGET).bin $(MDTARGET).lst $(TARGET).32x $(TARGET).lst

# Symbol listings for both CPUs
$(MDTARGET).lst: $(MDTARGET).elf
	$(MDNM) --plugin=$(MDPLUGIN) -n $< > $@

$(TARGET).lst: $(TARGET).elf
	$(SHNM) --plugin=$(SHPLUGIN) -n $< > $@

# m68k stuff

$(MDTARGET).bin: $(MDTARGET).elf
	@echo "Stripping ELF header from M68K program"
	@$(MDOBJC) -O binary $< $@

$(MDTARGET).elf: $(MDOBJS) md_src/md.ld
	$(MDCC) $(MDLDFLAGS) $(MDOBJS) -o $@ $(MDLIBS)

md_src/%.o: md_src/%.s
	@echo "MDAS $<"
	@$(MDCC) $(MDASFLAGS) -c $< -o $@

md_src/%.o: md_src/%.c
	@echo "MDCC $<"
	@$(MDCC) $(MDCCFLAGS) $(MDEXTRA) $(MDINCS) -MMD -MP -c $< -o $@

md_src/%.o: md_src/%.cpp
	@echo "MDCXX $<"
	@$(MDCXX) $(MDCXXFLAGS) $(MDEXTRA) $(MDINCS) -c $< -o $@

# sh2 stuff

sh_src/res/test_wide_data.h: sh_src/tools/convert_test_wide.py sh_src/tools/gdl_assets.py sh_src/tools/indexed_png.py sh_src/res/levels/Hub/fg_test_level_wide.png sh_src/res/levels/Hub/portal_starfield.png sh_src/res/sprites/Chakan/chakan_anim_no_weapons.png sh_src/src/levels/test/test_wide_collision.json
	$(PYTHON) sh_src/tools/convert_test_wide.py

sh_src/src/levels/test/test_wide.o: sh_src/res/test_wide_data.h


sh_src/res/portal_hub_data.h: sh_src/tools/convert_portal_hub.py sh_src/tools/gdl_assets.py sh_src/tools/indexed_png.py $(wildcard sh_src/res/levels/Hub/portal*.png) $(wildcard sh_src/res/sprites/Scenery/Hub/*.png) $(wildcard sh_src/res/sprites/Items/*.png) sh_src/res/sprites/Chakan/chakan_anim_no_weapons.png sh_src/src/levels/hub/portal_hub_collision.json
	$(PYTHON) sh_src/tools/convert_portal_hub.py

sh_src/src/levels/hub/portal_hub.o sh_src/src/levels/hub/hub_sprites.o sh_src/src/levels/hub/planets.o: sh_src/res/portal_hub_data.h

md_src/src/main.o sh_src/m_main.o sh_src/s_main.o: build/game_config.h

# SH-2 startup assembly embeds the M68000 startup binary.
# Ensure the M68000 binary exists before assembling mars_start.s.
sh_src/mars_start.o: $(MDTARGET).bin

$(TARGET).32x: $(TARGET).elf
	@echo "Stripping ELF header from SH-2 program"
	@$(SHOBJC) -O binary $< temp.32x
	@dd if=temp.32x of=$@ bs=8192 conv=sync
	@rm -f temp.32x

$(TARGET).elf: $(SHOBJS) sh_src/mars.ld
	$(SHCC) $(SHLDFLAGS) $(SHOBJS) -o $@ $(SHLIBS)

sh_src/%.o: sh_src/%.s 
	@echo "SHAS $<"
	@$(SHAS) $(SHASFLAGS) $< -o $@

sh_src/%.o: sh_src/%.c
	@echo "SHCC $<"
	@$(SHCC) $(SHCCFLAGS) $(SHEXTRA) $(SHINCS) -MMD -MP -c $< -o $@

sh_src/%.o: sh_src/%.cpp
	@echo "SHCXX $<"
	@$(SHCXX) $(SHCXXFLAGS) $(SHEXTRA) $(SHINCS) -c $< -o $@

.PHONY: clean

clean:
	rm -f $(MDOBJS) $(SHOBJS) $(MDCS:.c=.d) $(SHCS:.c=.d)
	rm -f sh_src/res/pause_data.h md_src/res/music_pause.bin sh_src/res/intro_data.h md_src/res/intro_lightning.bin md_src/res/menu_letter.bin md_src/res/menu_lightning.bin sh_src/res/menu_data.h md_src/res/music_intro.bin sh_src/res/logo_data.h md_src/res/logo_scream.bin md_src/res/test_small_data.h md_src/res/sprites.h sh_src/res/test_wide_data.h sh_src/res/portal_hub_data.h sh_src/res/t_earth1_data.h sh_src/res/t_air1_data.h sh_src/res/t_water1_data.h sh_src/res/t_fire1_data.h
	rm -f md_src/res/music_fire_dimension_1.bin md_src/res/music_water_dimension_1.bin md_src/res/music_air_dimension_1.bin md_src/res/music_stage_select.bin md_src/res/music_earth_dimension_1.bin md_src/res/drv_xgm.bin md_src/res/drv_xgm.lst
	rm -f $(MDTARGET).bin $(MDTARGET).elf $(MDTARGET).lst
	rm -f $(TARGET).32x $(TARGET).elf $(TARGET).lst
	rm -f m68k_crt0.bin.o m68k_crt0.bin

sh_src/res/t_earth1_data.h: sh_src/tools/convert_t_earth1.py sh_src/tools/gdl_assets.py sh_src/tools/indexed_png.py $(wildcard sh_src/res/levels/T_Earth/*.png) sh_src/res/sprites/Chakan/chakan_anim_no_weapons.png sh_src/src/levels/terrestrial/t_earth1_collision.json
	$(PYTHON) sh_src/tools/convert_t_earth1.py

sh_src/src/levels/terrestrial/earth_levels.o: sh_src/res/t_earth1_data.h

sh_src/res/t_air1_data.h: sh_src/tools/convert_t_air1.py sh_src/tools/gdl_assets.py sh_src/tools/indexed_png.py $(wildcard sh_src/res/levels/T_Air/*) sh_src/res/sprites/Chakan/chakan_anim_no_weapons.png sh_src/src/levels/terrestrial/air/t_air1_collision.json
	$(PYTHON) sh_src/tools/convert_t_air1.py

sh_src/src/levels/terrestrial/air/t_air_1.o: sh_src/res/t_air1_data.h

sh_src/res/t_water1_data.h: sh_src/tools/convert_t_water1.py sh_src/tools/gdl_assets.py sh_src/tools/indexed_png.py $(wildcard sh_src/res/levels/T_Water/*.png) sh_src/res/sprites/Chakan/chakan_anim_no_weapons.png sh_src/src/levels/terrestrial/water/t_water1_collision.json
	$(PYTHON) sh_src/tools/convert_t_water1.py

sh_src/src/levels/terrestrial/water/t_water_1.o: sh_src/res/t_water1_data.h

sh_src/res/t_fire1_data.h: sh_src/tools/convert_t_fire1.py sh_src/tools/gdl_assets.py sh_src/tools/indexed_png.py $(wildcard sh_src/res/levels/T_Fire/*) sh_src/res/sprites/Chakan/chakan_anim_no_weapons.png sh_src/src/levels/terrestrial/fire/t_fire1_collision.json
	$(PYTHON) sh_src/tools/convert_t_fire1.py

sh_src/src/levels/terrestrial/fire/t_fire_1.o: sh_src/res/t_fire1_data.h

sh_src/res/logo_data.h: sh_src/tools/convert_logo.py sh_src/tools/gdl_assets.py sh_src/tools/indexed_png.py sh_src/res/sgdk_retro_logo.png sh_src/res/sgdk_retro_logo_red.png
	$(PYTHON) sh_src/tools/convert_logo.py

sh_src/src/gamestates/gamestate_logo.o: sh_src/res/logo_data.h

sh_src/res/menu_data.h: sh_src/tools/convert_menu.py sh_src/tools/gdl_assets.py sh_src/tools/indexed_png.py $(wildcard sh_src/res/menus/*) $(wildcard sh_src/res/sprites/Menus/*)
	$(PYTHON) sh_src/tools/convert_menu.py

sh_src/src/gamestates/gamestate_menu.o: sh_src/res/menu_data.h

md_src/res/music_intro.bin: md_src/res/music/01\ -\ Intro\ Theme.vgm
	$(MDBIN)/xgmtool "$<" $@ -s -n

md_src/res/music.o: md_src/res/music_intro.bin

md_src/res/menu_lightning.bin: md_src/tools/convert_menu_sound.py md_src/tools/convert_logo_sound.py md_src/res/sfx/menu_lightning.wav
	$(PYTHON) md_src/tools/convert_menu_sound.py

md_src/res/music.o: md_src/res/menu_lightning.bin

sh_src/res/intro_data.h: sh_src/tools/convert_intro.py sh_src/tools/gdl_assets.py sh_src/tools/indexed_png.py $(wildcard sh_src/res/intro/*)
	$(PYTHON) sh_src/tools/convert_intro.py

sh_src/src/gamestates/gamestate_intro.o: sh_src/res/intro_data.h

md_src/res/intro_lightning.bin: md_src/tools/convert_logo_sound.py md_src/res/sfx/intro_lightning.wav
	$(PYTHON) -c "import sys; from pathlib import Path; sys.path.insert(0,'md_src/tools'); from convert_logo_sound import generate; Path('$@').write_bytes(generate(Path('md_src/res/sfx/intro_lightning.wav'),4))"

md_src/res/music.o: md_src/res/intro_lightning.bin

sh_src/res/pause_data.h: sh_src/tools/convert_pause.py sh_src/tools/indexed_png.py sh_src/tools/gdl_assets.py sh_src/res/menus/pause_menu.bmp
	$(PYTHON) sh_src/tools/convert_pause.py

sh_src/src/pause_menu.o: sh_src/res/pause_data.h

md_src/res/music_pause.bin: md_src/res/music/04\ -\ Item\ Selection.vgm
	$(MDBIN)/xgmtool "$<" $@ -s -n

md_src/res/music.o: md_src/res/music_pause.bin
