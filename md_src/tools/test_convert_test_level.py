"""Asset-format checks; run with python3 -m unittest discover -s tools."""
import struct
import tempfile
import unittest
import zlib
from pathlib import Path

from convert_test_level import convert, read_png


def chunk(kind, data):
    return (struct.pack('>I', len(data)) + kind + data
            + struct.pack('>I', zlib.crc32(kind + data) & 0xffffffff))


def png(rows, filter_type=0):
    width = len(rows[0])
    raw = bytearray()
    previous = [0] * width
    for row in rows:
        raw.append(filter_type)
        for x, value in enumerate(row):
            left = row[x - 1] if x else 0
            above = previous[x]
            corner = previous[x - 1] if x else 0
            p = left + above - corner
            # PNG's specified tie order: left, above, upper left.
            predictor = min((left, above, corner), key=lambda v: abs(p - v))
            predictor = [0, left, above, (left + above) // 2, predictor][filter_type]
            raw.append((value - predictor) & 255)
        previous = row
    palette = bytes(v for i in range(16) for v in (i * 16, i * 16, i * 16))
    return (b'\x89PNG\r\n\x1a\n'
            + chunk(b'IHDR', struct.pack('>IIBBBBB', width, len(rows), 8, 3, 0, 0, 0))
            + chunk(b'PLTE', palette)
            + chunk(b'IDAT', zlib.compress(raw)) + chunk(b'IEND', b''))


class ConversionTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.path = Path(self.temp.name) / 'test.png'

    def test_all_png_filters(self):
        rows = [[(x * 3 + y * 5) % 16 for x in range(8)] for y in range(8)]
        for mode in range(5):
            with self.subTest(filter=mode):
                self.path.write_bytes(png(rows, mode))
                self.assertEqual(read_png(self.path)[2], rows)

    def test_tile_order_packing_and_deduplication(self):
        rows = [[1] * 8 + [2] * 8 for _ in range(8)] * 2
        self.path.write_bytes(png(rows))
        width, height, tiles, words, tilemap, palette = convert(self.path)
        self.assertEqual((width, height), (2, 2))
        self.assertEqual(len(tiles), 2)
        self.assertEqual(words, [0x1111] * 16 + [0x2222] * 16)
        self.assertEqual(tilemap, [0, 1, 0, 1])
        self.assertEqual(palette[14], 0x0EEE)

    def test_reject_palette_overflow(self):
        self.path.write_bytes(png([[16] * 8 for _ in range(8)]))
        with self.assertRaisesRegex(ValueError, '16-color'):
            read_png(self.path)

    def test_reject_corrupt_chunk(self):
        data = bytearray(png([[0] * 8 for _ in range(8)]))
        data[20] ^= 1
        self.path.write_bytes(data)
        with self.assertRaisesRegex(ValueError, 'corrupt'):
            read_png(self.path)


if __name__ == '__main__':
    unittest.main()
