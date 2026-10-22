"""Air 1 asset fidelity, tile priority, collision and autonomous band scrolling."""
import json
import os
import re
import subprocess
import tempfile
import unittest
from pathlib import Path
from convert_t_air1 import generate
from indexed_png import read_png
ROOT = Path(__file__).resolve().parent.parent


class Air1Tests(unittest.TestCase):
    def test_original_art_and_collision(self):
        output = generate()
        def values(name):
            body = re.search(r'air_' + name + r'\[\] = \{(.*?)\};', output, re.S)[1]
            return [int(v) for v in re.findall(r'\d+', body)]
        _, _, rows, _ = read_png(ROOT / 'res/levels/T_Air/T_Air_1_fg.png', 256)
        tiles, offsets, tilemap = values('fg_tiles'), values('fg_rows'), values('fg_map')
        priority = values('fg_priority')
        for y, source in enumerate(rows):
            actual = []
            for x in range(240):
                offset = offsets[tilemap[(y//8)*240+x]+y%8]
                actual.extend((word>>shift)&255 for word in tiles[offset:offset+2] for shift in (24,16,8,0))
            self.assertEqual(actual, [(v&31) if v&15 else 0 for v in source])
        for ty in range(180):
            for tx in range(240):
                expected = any(v&128 for row in rows[ty*8:ty*8+8] for v in row[tx*8:tx*8+8])
                i = ty*240+tx
                self.assertEqual(bool(priority[i>>3] & (1<<(i&7))), expected)
        self.assertTrue(any(priority))
        _, _, bg_rows, _ = read_png(ROOT / 'res/levels/T_Air/T_Air_1_bg_wide.png')
        bg_tiles, bg_map = values('bg_tiles'), values('bg_map')
        for y, source in enumerate(bg_rows):
            actual = []
            for x in range(32):
                offset = bg_map[(y//8)*32+x]+(y%8)*2
                actual.extend((word>>shift)&255 for word in bg_tiles[offset:offset+2] for shift in (24,16,8,0))
            self.assertEqual(actual,[v+48 for v in source])
        data = json.loads((ROOT / 'src/levels/terrestrial/air/t_air1_collision.json').read_text())
        self.assertEqual(data['spawn'], [464,64])
        self.assertEqual(len(data['rows_rle']),90)
        for row in data['rows_rle']:
            for run in row:
                self.assertRegex(run,r'^0x[0-9A-F]{4}$')
                self.assertLess(int(run,16)>>8,5)
            self.assertEqual(sum(int(run,16)&255 for run in row),120)
        self.assertEqual(len(values('palette')),64)

    def test_level_scroll_and_solid_heightmap(self):
        with tempfile.TemporaryDirectory() as temp:
            (Path(temp)/'t_air1_data.h').write_text(generate())
            binary=str(Path(temp)/'air-test')
            sources=['tools/tests/air_level_test.c','src/levels/terrestrial/air/t_air_1.c',
                     'src/player.c','src/input_controls.c','src/player_sprite.c','src/camera.c',
                     'src/level_collision/collision.c','src/level_collision/collision_heightmaps.c']
            subprocess.run([os.environ.get('HOST_CC','cc'),'-std=c99','-O2','-Wall','-Wextra','-Werror',
                            '-fsanitize=undefined','-fno-sanitize-recover=undefined',
                            '-I'+temp,'-I'+str(ROOT/'inc'),'-I'+str(ROOT/'src'),
                            *[str(ROOT/s) for s in sources],'-o',binary],check=True)
            subprocess.run([binary],check=True)
