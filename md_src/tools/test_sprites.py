"""Verify generated SAT pieces reconstruct every original animation frame."""
import re
import unittest
from convert_sprites import generate
from convert_test_level import ROOT, read_png


class SpriteTests(unittest.TestCase):
    def test_animation_reconstruction(self):
        output = generate()
        def body(name):
            return output.split(name + '[] = {', 1)[1].split('};', 1)[0]
        words = [int(v, 16) for v in re.findall(r'0x[0-9A-F]+', body('chakan_tiles'))]
        pixels = []
        for start in range(0, len(words), 16):
            tile = []
            for word in words[start:start + 16]:
                tile.extend((word >> shift) & 15 for shift in (12, 8, 4, 0))
            pixels.append(tile)
        offsets = [int(v) for v in re.findall(r'\d+', body('chakan_frame_offsets'))]
        pieces = [tuple(map(int, v)) for v in re.findall(r'\{(\d+), (\d+), (\d+)\}', body('chakan_pieces'))]
        _, _, rows, _ = read_png(ROOT / 'res/sprites/Chakan/chakan_anim_no_weapons.png')
        frame_index = 0
        for animation in range(5):
            for frame in range(8 if animation == 1 else 1):
                image = [[0] * 112 for _ in range(80)]
                entries = pieces[offsets[frame_index]:offsets[frame_index + 1]]
                self.assertLessEqual(len(entries), 80)
                for x, y, tile in entries:
                    for dy in range(8):
                        image[y + dy][x:x + 8] = pixels[tile][dy * 8:dy * 8 + 8]
                expected = [row[frame * 112:(frame + 1) * 112]
                            for row in rows[animation * 80:(animation + 1) * 80]]
                self.assertEqual(image, expected, (animation, frame))
                frame_index += 1
        self.assertEqual(frame_index, 12)
