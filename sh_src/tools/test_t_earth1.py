"""Verify Earth 1 asset preservation and execute the SH-2 collision code."""
import json
import os
import re
import subprocess
import tempfile
import unittest
from pathlib import Path
from convert_t_earth1 import generate
from indexed_png import read_png
ROOT = Path(__file__).resolve().parent.parent


class Earth1Tests(unittest.TestCase):
    def test_collision_and_terrain(self):
        data = json.loads((ROOT / 'src/levels/terrestrial/t_earth1_collision.json').read_text())
        self.assertEqual((data['width'], data['height']), (161, 54))
        self.assertEqual(data['spawn'], [0, 160])
        for row in data['rows_rle']:
            for run in row:
                self.assertRegex(run, r'^0x[0-9A-F]{4}$')
            self.assertEqual(sum(int(run, 16) & 255 for run in row), 161)
        ids = {int(run, 16) >> 8 for row in data['rows_rle'] for run in row}
        self.assertTrue({8, 9}.issubset(ids))
        output = generate()
        def values(name):
            body = re.search(r'earth_' + name + r'\[\] = \{(.*?)\};', output, re.S)[1]
            return [int(v) for v in re.findall(r'\d+', body)]
        _, _, rows, _ = read_png(ROOT / 'res/levels/T_Earth/T_Earth_1_fg.png')
        tiles, offsets, tilemap = values('fg_tiles'), values('fg_rows'), values('fg_map')
        for y, source in enumerate(rows):
            reconstructed = []
            for x in range(322):
                offset = offsets[tilemap[(y // 8) * 322 + x] + y % 8]
                reconstructed.extend((word >> shift) & 255 for word in tiles[offset:offset + 2] for shift in (24, 16, 8, 0))
            self.assertEqual(reconstructed, source)
        _, _, rows, _ = read_png(ROOT / 'res/levels/T_Earth/T_Earth_1_bg.png')
        tiles, tilemap = values('bg_tiles'), values('bg_map')
        for y, source in enumerate(rows):
            reconstructed = []
            for x in range(32):
                offset = tilemap[(y // 8) * 32 + x] + (y % 8) * 2
                reconstructed.extend((word >> shift) & 255 for word in tiles[offset:offset + 2] for shift in (24, 16, 8, 0))
            self.assertEqual(reconstructed, [v + 16 for v in source])

    def test_crouch_and_roll_frames(self):
        from gdl_assets import pack_spans
        _, _, sheet, _ = read_png(ROOT / 'res/sprites/Chakan/chakan_anim_no_weapons.png')
        frames = [(0, 400)] + [(f * 112, 480) for f in range(4)]
        offsets, spans, pixels = pack_spans(sheet, 112, 80, frames, 32)
        index = 0
        for flip in (False, True):
            for fx, fy in frames:
                for y in range(80):
                    expected = [v + 32 if v else 0 for v in sheet[fy+y][fx:fx+112]]
                    if flip:
                        expected.reverse()
                    actual = [0] * 112
                    for span in range(offsets[index], offsets[index+1]):
                        x, count, offset = spans[span*3:span*3+3]
                        actual[x:x+count] = pixels[offset:offset+count]
                    self.assertEqual(actual, expected)
                    index += 1

    def test_heightmap_movement(self):
        with tempfile.TemporaryDirectory() as temp:
            binary = str(Path(temp) / 'heightmap-test')
            subprocess.run([os.environ.get('HOST_CC', 'cc'), '-std=c99', '-O2', '-Wall', '-Wextra', '-Werror',
                            '-fsanitize=undefined', '-fno-sanitize-recover=undefined', '-I' + str(ROOT / 'inc'),
                            str(ROOT / 'tools/tests/heightmap_collision_test.c'), str(ROOT / 'src/player.c'), str(ROOT / 'src/input_controls.c'), str(ROOT / 'src/player_sprite.c'),
                            str(ROOT / 'src/level_collision/collision.c'),
                            str(ROOT / 'src/level_collision/collision_heightmaps.c'), '-o', binary], check=True)
            subprocess.run([binary], check=True)

    def test_independent_camera_driven_scroll(self):
        with tempfile.TemporaryDirectory() as temp:
            binary = str(Path(temp) / 'earth-scroll-test')
            (Path(temp) / 't_earth1_data.h').write_text(generate())
            subprocess.run([os.environ.get('HOST_CC', 'cc'), '-std=c99', '-O2', '-Wall', '-Wextra', '-Werror',
                            '-fsanitize=undefined', '-fno-sanitize-recover=undefined',
                            '-I' + temp, '-I' + str(ROOT / 'inc'), '-I' + str(ROOT / 'src'),
                            str(ROOT / 'tools/tests/earth_scroll_test.c'),
                            str(ROOT / 'src/levels/terrestrial/earth_levels.c'),
                            str(ROOT / 'src/player.c'), str(ROOT / 'src/input_controls.c'),
                            str(ROOT / 'src/player_sprite.c'), str(ROOT / 'src/camera.c'),
                            str(ROOT / 'src/level_collision/collision.c'),
                            str(ROOT / 'src/level_collision/collision_heightmaps.c'), '-o', binary], check=True)
            subprocess.run([binary], check=True)
