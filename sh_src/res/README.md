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
