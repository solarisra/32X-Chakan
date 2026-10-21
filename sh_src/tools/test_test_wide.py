"""Host checks for SH-2 resources and portable gameplay/camera code."""
import json
import os
import re
import subprocess
import tempfile
import unittest
from pathlib import Path
from convert_test_wide import ROOT, convert_image, generate
from indexed_png import read_png


class WideTests(unittest.TestCase):
    def test_tile_reconstruction(self):
        for filename in ('fg_test_level_wide.png', 'portal_starfield.png'):
            w, h, rows, palette, tiles, tilemap = convert_image(ROOT / 'res/levels/Hub' / filename)
            for y in range(h):
                reconstructed = []
                for x in range(w // 8):
                    tile = tiles[tilemap[(y // 8) * (w // 8) + x]]
                    reconstructed.extend(tile[(y & 7) * 8:(y & 7) * 8 + 8])
                self.assertEqual(reconstructed, rows[y])

    def test_packed_tiles_and_transparency(self):
        output = generate()
        def values(name):
            body = output.split(name + '[] = {', 1)[1].split('};', 1)[0]
            return [int(v) for v in re.findall(r'\d+', body)]
        for name, filename in [('wide_fg', 'fg_test_level_wide.png'), ('wide_bg', 'portal_starfield.png')]:
            _, _, _, _, tiles, tilemap = convert_image(ROOT / 'res/levels/Hub' / filename)
            packed = values(name + '_tiles')
            stride = 40 if name == 'wide_fg' else 16
            self.assertEqual(values(name + '_map'), [tile * stride for tile in tilemap])
            for index, tile in enumerate(tiles):
                for y in range(8):
                    row = packed[index * stride + y * (stride // 8):]
                    pixels = [(word >> shift) & 255 for word in row[:2] for shift in (24, 16, 8, 0)]
                    original = list(tile[y * 8:y * 8 + 8])
                    self.assertEqual(pixels, [v + (16 if name == 'wide_bg' else 0) for v in original])
                    if name == 'wide_fg':
                        mask = [(word >> shift) & 255 for word in row[2:4] for shift in (24, 16, 8, 0)]
                        self.assertEqual(mask, [255 if v == 0 else 0 for v in original])
                        self.assertEqual(row[4], 0 if not any(original) else 1 if all(original) else 2)

    def test_sprite_spans_both_facings(self):
        output = generate()
        def values(name):
            body = output.split(name + '[] = {', 1)[1].split('};', 1)[0]
            return [int(v) for v in re.findall(r'\d+', body)]
        offsets = values('wide_sprite_rows')
        spans = values('wide_sprite_spans')
        pixels = values('wide_sprite_pixels')
        self.assertLess(len(pixels), 65536)
        _, _, sheet, _ = read_png(ROOT / 'res/sprites/Chakan/chakan_anim_no_weapons.png')
        row_index = 0
        for flip in (False, True):
            for animation in range(5):
                for frame in range(8 if animation == 1 else 1):
                    for y in range(80):
                        actual = [0] * 112
                        for span in range(offsets[row_index], offsets[row_index + 1]):
                            x, count, data = spans[span * 3:span * 3 + 3]
                            self.assertLessEqual(x + count, 112)
                            actual[x:x + count] = pixels[data:data + count]
                        expected = [v + 32 if v else 0 for v in sheet[animation * 80 + y][frame * 112:(frame + 1) * 112]]
                        if flip:
                            expected.reverse()
                        self.assertEqual(actual, expected)
                        row_index += 1
        self.assertEqual(row_index, 1920)

    def test_collision_rows(self):
        data = json.loads((ROOT / 'src/levels/test/test_wide_collision.json').read_text())
        self.assertEqual(len(data['rows_rle']), 15)
        for encoded_row in data['rows_rle']:
            for run in encoded_row:
                self.assertRegex(run, r'^0x[0-9A-F]{4}$')
            row = [int(run, 16) for run in encoded_row]
            self.assertEqual(sum(run & 255 for run in row), 90)
            self.assertTrue(all(run >> 8 in (0, 1) for run in row))
        self.assertEqual(data['rows_rle'][-2:], [['0x005A'], ['0x005A']])

    def test_sh2_movement_and_camera(self):
        with tempfile.TemporaryDirectory() as temp:
            binary = str(Path(temp) / 'wide-test')
            compiler = os.environ.get('HOST_CC', 'cc')
            flags = ['-std=c99', '-Wall', '-Wextra', '-Werror', '-fsanitize=undefined',
                     '-fno-sanitize-recover=undefined', '-I' + str(ROOT / 'inc')]
            subprocess.run([compiler, *flags,
                            str(ROOT.parent / 'md_src/tools/tests/player_test.c'),
                            str(ROOT / 'src/player.c'), str(ROOT / 'src/input_controls.c'),
                            '-o', binary], check=True)
            subprocess.run([binary], check=True)
            harness = Path(temp) / 'camera.c'
            harness.write_text('''#include <assert.h>
#include "camera.h"
int main(void) {
 camera_init(1440,240);
 camera_step(100,80);
 assert(cameraPosition.x==0 && cameraPosition.y==0);
 camera_step(200,140);
 assert(cameraPosition.x==36 && cameraPosition.y==16);
 camera_step(200,140);
 assert(cameraPosition.x==36 && cameraPosition.y==16);
 camera_step(1440,240);
 assert(cameraPosition.x==1120 && cameraPosition.y==16);
 camera_step(0,0);
 assert(cameraPosition.x==0 && cameraPosition.y==0);
 return 0;
}
''')
            subprocess.run([compiler, *flags, str(harness), str(ROOT / 'src/camera.c'),
                            '-o', binary], check=True)
            subprocess.run([binary], check=True)


if __name__ == '__main__':
    unittest.main()
