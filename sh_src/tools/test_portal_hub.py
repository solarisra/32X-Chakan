"""Verify Hub palette, compact terrain, collision and scenery encodings."""
import json
import unittest
from indexed_png import read_png
from gdl_assets import pack_compact_foreground, pack_spans
from convert_portal_hub import ROOT


class PortalHubTests(unittest.TestCase):
    def test_compact_terrain_matches_original(self):
        w, h, raw, _ = read_png(ROOT / 'res/levels/Hub/portal_hub_fg1.png', 32)
        self.assertEqual((w, h), (1920, 1280))
        rows = [[v if v & 15 else 0 for v in row] for row in raw]
        packed, offsets, tilemap = pack_compact_foreground(rows)
        self.assertEqual(len(tilemap), 240 * 160)
        for ty in range(160):
            for tx in range(240):
                tile = tilemap[ty * 240 + tx]
                for dy in range(8):
                    row = packed[offsets[tile + dy]:offsets[tile + dy] + 5]
                    actual = [(word >> shift) & 255 for word in row[:2] for shift in (24, 16, 8, 0)]
                    expected = rows[ty * 8 + dy][tx * 8:tx * 8 + 8]
                    self.assertEqual(actual, expected)
                    self.assertEqual(row[4], 0 if not any(expected) else 1 if all(expected) else 2)
        _, _, _, pal0 = read_png(ROOT / 'res/levels/Hub/portalhub_pal0.png', tiled=False)
        _, _, _, pal1 = read_png(ROOT / 'res/levels/Hub/portalhub_pal1.png', tiled=False)
        _, _, _, combined = read_png(ROOT / 'res/levels/Hub/portal_hub_fg1.png', 32)
        # Explicit SGDK runtime palettes from create_level_stages 07e5f08.
        # PAL0 intentionally overrides the foreground PNG's embedded entry 2.
        expected0=[0xE0E,0,0x08C,0x246,0x400,0x020,0x8CC,0x200,
                   0x4AA,0x468,0x224,0x00E,0x00A,0x02C,0x06C,0x0AE]
        expected1=[0xE0E,0,0x002,0x022,0x004,0x024,0x046,0x068,
                   0x08A,0x4AC,0,0xA00,0x800,0x600,0x800,0xA00]
        self.assertEqual(pal0,expected0)
        self.assertEqual(pal1,expected1)
        self.assertEqual(combined[:2]+combined[3:],(pal0+pal1)[:2]+(pal0+pal1)[3:])

    def test_original_collision_and_doors(self):
        data = json.loads((ROOT / 'src/levels/hub/portal_hub_collision.json').read_text())
        self.assertEqual(data['spawn'], [756, 720])
        self.assertEqual(len(data['rows_rle']), 80)
        grid = []
        for encoded in data['rows_rle']:
            row = []
            for encoded_run in encoded:
                self.assertRegex(encoded_run, r"^0x[0-9A-F]{4}$")
                run = int(encoded_run, 16)
                self.assertLessEqual(run >> 8, 6)
                row.extend([run >> 8] * (run & 255))
            self.assertEqual(len(row), 120)
            grid.append(row)
        self.assertEqual(grid[12][25], 3)
        self.assertEqual(grid[22][108], 4)
        self.assertEqual(grid[72][38], 5)
        self.assertEqual(grid[60][111], 6)
        self.assertEqual(grid[50][50], 1)  # Floor beneath original spawn.

    def test_scenery_spans_both_facings(self):
        for filename, width, height, count, base in [
            ('flame.png', 16, 32, 6, 0),
            ('T_Earth_Portal_single.png', 64, 88, 1, 16),
            ('T_Fire_Portal_single.png', 64, 88, 1, 16),
            ('T_Water_Portal_single.png', 64, 88, 1, 16),
            ('T_Air_Portal_single.png', 64, 88, 1, 16),
            ('green_planet.png',32,32,1,64),
            ('green_planets_small.png',24,24,1,64),
            ('planet_w_moons.png',48,48,1,64),
            ('ringed_planet.png',96,40,1,64),
            ('../../Items/potion_earth.png',16,16,4,32),
            ('../../Items/potion_fire.png',16,16,4,32),
            ('../../Items/potion_water.png',16,16,4,32),
            ('../../Items/potion_air.png',16,24,4,32),
            ('../../Items/hourglass_full.png',24,32,1,32),
        ]:
            _, _, rows, _ = read_png(ROOT / 'res/sprites/Scenery/Hub' / filename)
            offsets, spans, pixels = pack_spans(rows, width, height, [(f * width, 0) for f in range(count)], base)
            index = 0
            for flip in (False, True):
                for frame in range(count):
                    for y in range(height):
                        expected = [v + base if v else 0 for v in rows[y][frame * width:(frame + 1) * width]]
                        if flip: expected.reverse()
                        actual = [0] * width
                        for span in range(offsets[index], offsets[index + 1]):
                            x, length, offset = spans[span * 3:span * 3 + 3]
                            actual[x:x + length] = pixels[offset:offset + length]
                        self.assertEqual(actual, expected)
                        index += 1


if __name__ == '__main__':
    unittest.main()
