"""Indexed-PNG decoder for SH-2 resource conversion (standard library only)."""
import struct
import zlib


def read_png(path, max_colors=16, tiled=True, rgb_palette=None, embedded_palette=False):
    data = path.read_bytes()
    if data[:8] != b'\x89PNG\r\n\x1a\n':
        raise ValueError(f'{path}: invalid PNG signature')
    pos, palette, compressed = 8, rgb_palette, bytearray()
    while pos < len(data):
        size = struct.unpack_from('>I', data, pos)[0]
        kind = data[pos + 4:pos + 8]
        chunk = data[pos + 8:pos + 8 + size]
        crc = struct.unpack_from('>I', data, pos + 8 + size)[0]
        if zlib.crc32(kind + chunk) & 0xffffffff != crc:
            raise ValueError(f'{path}: corrupt {kind!r} chunk')
        if kind == b'IHDR':
            width, height, depth, color, compression, filtering, interlace = struct.unpack('>IIBBBBB', chunk)
            if depth not in (1,2,4,8) or (color, compression, filtering, interlace) != (3 if rgb_palette is None else 2, 0, 0, 0):
                raise ValueError(f'{path}: expected noninterlaced indexed PNG')
        elif kind == b'PLTE':
            palette = list(zip(chunk[::3], chunk[1::3], chunk[2::3]))
        elif kind == b'IDAT':
            compressed.extend(chunk)
        elif kind == b'IEND':
            break
        pos += size + 12
    if (tiled and (width % 8 or height % 8)) or palette is None:
        raise ValueError(f'{path}: invalid tile dimensions or missing palette')
    channels = 3 if color == 2 else 1
    stride = (width * depth * channels + 7) // 8
    raw = zlib.decompress(compressed)
    if len(raw) != (stride + 1) * height:
        raise ValueError(f'{path}: unexpected decoded size')
    rows, previous = [], [0] * stride
    for y in range(height):
        offset = y * (stride + 1)
        filter_type = raw[offset]
        row = list(raw[offset + 1:offset + 1 + stride])
        if filter_type > 4:
            raise ValueError(f'{path}: invalid PNG filter')
        for x in range(stride):
            left = row[x - channels] if x >= channels else 0
            above = previous[x]
            corner = previous[x - channels] if x >= channels else 0
            p = left + above - corner
            distances = (abs(p - left), abs(p - above), abs(p - corner))
            paeth = (left, above, corner)[distances.index(min(distances))]
            predictor = (0, left, above, (left + above) // 2, paeth)[filter_type]
            row[x] = (row[x] + predictor) & 255
        if color == 2:
            rows.append([tuple(row[x:x+3]) for x in range(0,stride,3)])
        else:
            rows.append([(row[x * depth // 8] >> (8 - depth - x * depth % 8)) & ((1 << depth)-1) for x in range(width)])
        previous = row
    if color == 2:
        if embedded_palette:
            palette = [rows[(i//16)*8][(i%16)*8] for i in range(max_colors)]
            rows = rows[32:]
            height -= 32
        lookup = {}
        for i,c in enumerate(palette): lookup.setdefault(c,i)
        if embedded_palette and any(c not in lookup for row in rows for c in row):
            raise ValueError(f'{path}: image color missing from embedded palette')
        rows = [[lookup[c] if c in lookup else min(range(len(palette)), key=lambda i: sum((palette[i][k]-c[k])**2 for k in range(3))) for c in row] for row in rows]
    if max(max(row) for row in rows) >= min(max_colors, len(palette)):
        raise ValueError(f'{path}: palette indices exceed {max_colors} colors')
    colors = [(b >> 5) << 9 | (g >> 5) << 5 | (r >> 5) << 1 for r, g, b in palette[:max_colors]]
    return width, height, rows, colors + [0] * (max_colors - len(colors))

