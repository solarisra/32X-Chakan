"""Build-time packing primitives for GDL32X's version 1 resource format."""
def pack_tiles(rows, foreground=True, palette_base=0):
    height, width = len(rows), len(rows[0])
    unique, lookup, tilemap = [], {}, []
    for y in range(0, height, 8):
        for x in range(0, width, 8):
            tile = tuple(v for row in rows[y:y + 8] for v in row[x:x + 8])
            if tile not in lookup:
                lookup[tile] = len(unique)
                unique.append(tile)
            tilemap.append(lookup[tile])
    packed = []
    for tile in unique:
        for y in range(8):
            pixels = tile[y * 8:y * 8 + 8]
            packed.extend(sum((v + palette_base) << (24 - i * 8) for i, v in enumerate(pixels[start:start + 4])) for start in (0, 4))
            if foreground:
                packed.extend(sum((255 if not v else 0) << (24 - i * 8) for i, v in enumerate(pixels[start:start + 4])) for start in (0, 4))
                packed.append(0 if not any(pixels) else 1 if all(pixels) else 2)
    stride = 40 if foreground else 16
    if len(unique) * stride > 65536:
        raise ValueError('GDL tile offsets exceed uint16_t range')
    return packed, [v * stride for v in tilemap]


def pack_spans(rows, width, height, frames, palette_base=0):
    offsets, spans, pixels = [0], [], []
    shared_runs = {}
    for flip in (False, True):
        for fx, fy in frames:
            for y in range(height):
                row = rows[fy + y][fx:fx + width]
                if flip:
                    row = row[::-1]
                x = 0
                while x < width:
                    if not row[x]:
                        x += 1
                        continue
                    start = x
                    while x < width and row[x]:
                        x += 1
                    run = tuple(v + palette_base for v in row[start:x])
                    if run not in shared_runs:
                        shared_runs[run] = len(pixels)
                        pixels.extend(run)
                    spans.extend((start, x - start, shared_runs[run]))
                offsets.append(len(spans) // 3)
    if len(pixels) > 65535 or len(spans) // 3 > 65535:
        raise ValueError('GDL sprite offsets exceed uint16_t range')
    return offsets, spans, pixels


def native_palette(colors):
    return [(((c >> 1) & 7) * 31 // 7) | ((((c >> 5) & 7) * 31 // 7) << 5)
            | ((((c >> 9) & 7) * 31 // 7) << 10) for c in colors]


def pack_compact_foreground(rows):
    packed, tilemap = pack_tiles(rows)
    unique_rows, row_lookup, indices = [], {}, []
    for i in range(0, len(packed), 5):
        row = tuple(packed[i:i + 5])
        if row not in row_lookup:
            row_lookup[row] = len(unique_rows)
            unique_rows.extend(row)
        indices.append(row_lookup[row])
    if len(unique_rows) > 65536:
        raise ValueError('compact row offsets exceed uint16_t range')
    return unique_rows, indices, [v // 5 for v in tilemap]
