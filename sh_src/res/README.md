TEST_WIDE resources preserve their original Chakan_Remake_v1 paths:

- `levels/Hub/fg_test_level_wide.png`
- `levels/Hub/portal_starfield.png`
- `sprites/Chakan/chakan_anim_no_weapons.png`

All PNGs are unchanged copies. `../tools/convert_test_wide.py` generates the
ignored `test_wide_data.h` using Python's standard library and the local
indexed-PNG decoder in `sh_src/tools/indexed_png.py`. The game does not
need the sibling SGDK checkout to build.

Collision rows and spawn from original `src/levels/test/test_wide.c` are recorded
in `../src/levels/test/test_wide_collision.json`. The last two all-empty rows
were corrected from a 144-cell run to the declared 90-cell map width. The
source artwork retains its original ownership and provenance.

Portal Hub assets now also preserve `levels/Hub/portal_hub_fg1.png`, its two
palette PNGs, and the flame and four portal PNGs under `sprites/Scenery/Hub/`.
All images are unchanged copies from Chakan_Remake_v1. `convert_portal_hub.py`
builds `portal_hub_data.h` using the common packing helpers in `gdl_assets.py`.
`src/levels/hub/portal_hub_collision.json` records the original collision RLE
verbatim, including non-solid door IDs. No sibling checkout is needed to build.

Terrestrial Earth 1 preserves `levels/T_Earth/T_Earth_1_fg.png` and
`levels/T_Earth/T_Earth_1_bg.png` unchanged. `convert_t_earth1.py` generates
the ignored `t_earth1_data.h`. Its original collision rows remain four-digit
hexadecimal strings under `src/levels/terrestrial/t_earth1_collision.json`.
Art and collision are 161 columns wide despite the original level metadata
declaring 160; the port preserves all columns. The Earth Dimension 1 VGM
is copied unchanged under `md_src/res/music/` for the Genesis audio service.

The Chakan sheet now comes from `create_level_stages` (896x560), including
crouch and four roll poses. Converters retain the original twelve movement
frames in SDRAM and encode five supplemental action frames for ROM access.
Earth 1 collision RLE matches this branch, including one-way IDs `0x08`/`0x09`.

T_Air_1 assets preserve `levels/T_Air/T_Air_1_fg.png`,
`T_Air_1_bg_wide.png` and both JASC `.pal` files unchanged from
`create_level_stages`. `convert_t_air1.py` generates `t_air1_data.h`.
Foreground bit 7 becomes a tile-priority bitset; palette-bank indices 0..31
remain separate from Chakan (32..47) and the background (48..63).
`src/levels/terrestrial/air/t_air1_collision.json` preserves all 90 source RLE
rows as hexadecimal strings. The VGM stays under `md_src/res/music/`.

T_Water_1 preserves `levels/T_Water/T_Water_1_fg_w_prio_new.png` and
`T_Water_1_bg_w_prio.png`. `convert_t_water1.py` creates `t_water1_data.h`,
including compact foreground/background row dictionaries, priority maps, player
frames and the source's 31 hexadecimal collision rows. The priority maps model
SGDK's enabled base priority addition, so resource bit 7 is inverted for Water.
All raster image data fits in SDRAM by keeping immutable Chakan span descriptors
in ROM. The Water Dimension 1 VGM stays at its original relative music path.

### Terrestrial Fire 1

`levels/T_Fire/T_Fire_1_fg.png`, `pal0_t_fire1_fg.pal`, and
`pal1_t_fire1_fg.pal` are unchanged copies from Chakan_Remake_v1. The original
level uses no background map. `tools/convert_t_fire1.py` packs the two foreground
palette banks and tile priority flags, shared Chakan animations, and hexadecimal
collision RLE into the generated, ignored `t_fire1_data.h`. The original Fire 1
music remains under `md_src/res/music/05 - Fire Dimension 1.vgm`.

### Portal Hub items and planets

Original potion strips and `hourglass_full.png` are under `sprites/Items/`.
The four planet PNGs retain their original `sprites/Scenery/Hub/` paths.
`convert_portal_hub.py` packs these as ROM-backed spans: items use the original
Chakan palette bank, planets receive their own palette at indices 64..79.
The original Hub supplies potion/hourglass placements. The planet layout uses
two horizontal instances per type, spaced across the full Hub width, with vertical wrapping
at the level boundaries.

### STATE_LOGO

`sgdk_retro_logo.png` and `sgdk_retro_logo_red.png` are unchanged source assets.
`tools/convert_logo.py` generates ignored `logo_data.h` with the original blue
sprite pixels, both palettes, and a black 32X scene. The red PNG supplies only
its palette, as in SGDK. The original scream WAV is preserved under
`md_src/res/sfx/`; its build converter targets the existing XGM1 PCM driver.
