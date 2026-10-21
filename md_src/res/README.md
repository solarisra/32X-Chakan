These assets were copied from the user's Chakan_Remake_v1 checkout, preserving
its image paths under `md_src`:

- `levels/Hub/fg_test_level.png`: original `res/levels/Hub/fg_test_level.png`
- `levels/Hub/portal_starfield.png`: original `res/levels/Hub/portal_starfield.png`

`../src/levels/test/test_small_collision.json` records the RLE rows, dimensions
and spawn from the original `src/levels/test/test_small.c`. Each word stores
tile ID in the high byte and run length in the low byte. This scene uses only
empty (0) and solid (1).

The PNGs are unchanged. `md_src/tools/convert_test_level.py` uses Python 3 and
no third-party packages to generate `md_src/res/test_small_data.h`. The port
build does not depend on the sibling SGDK checkout. The source artwork retains
its original ownership and provenance.

`sprite` resources now also include `sprites/Chakan/chakan_anim_no_weapons.png`,
copied unchanged from the same relative source path. `md_src/tools/convert_sprites.py`
generates `md_src/res/sprites.h` with animation tiles and SAT pieces.

`music/02 - Stage Select.vgm` is copied unchanged from the original music
resources. MarsDev's `xgmtool` generates `music_stage_select.bin` (compiled XGC);
`sjasm` generates `drv_xgm.bin` from the vendored SGDK driver sources. `music.s`
embeds these generated resources in the 68000 ROM image.
