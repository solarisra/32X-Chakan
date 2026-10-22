"""Water asset fidelity, compact backgrounds, raster bands and collision flags."""
import json
import os
import re
import subprocess
import tempfile
import unittest
from pathlib import Path
from convert_t_water1 import generate
from indexed_png import read_png
ROOT=Path(__file__).resolve().parent.parent


class Water1Tests(unittest.TestCase):
    def test_original_art_and_rle(self):
        output=generate()
        def values(name):
            body=re.search(r'water_'+name+r'\[\] = \{(.*?)\};',output,re.S)[1]
            return [int(v) for v in re.findall(r'\d+',body)]
        for name,filename,width,height,base,stride in [
            ('fg','T_Water_1_fg_w_prio_new.png',800,62,0,5),
            ('bg','T_Water_1_bg_w_prio.png',64,62,16,2),
        ]:
            _,_,rows,_=read_png(ROOT/'res/levels/T_Water'/filename,256)
            tiles,offsets,tilemap=values(name+'_tiles'),values(name+'_rows'),values(name+'_map')
            bits=values(name+'_priority')
            if name=='fg':
                for i,entry in enumerate(tilemap):
                    self.assertEqual(bool(entry&32768),bool(bits[i>>3]&(1<<(i&7))))
            opaque = values('bg_opaque_rows') if name=='bg' else None
            for y,source in enumerate(rows):
                actual=[]
                for x in range(width):
                    entry=tilemap[(y//8)*width+x]
                    offset=offsets[(entry&32767 if name=='fg' else entry)+y%8]
                    actual.extend((word>>shift)&255 for word in tiles[offset:offset+2] for shift in (24,16,8,0))
                expected=[]
                for x,v in enumerate(source):
                    high=bits[((y//8)*width+x//8)>>3] & (1<<(((y//8)*width+x//8)&7))
                    expected.append((v&15)+base+(128 if name=='bg' and high and v&15 else 0))
                self.assertEqual(actual,expected)
            if opaque is not None:
                for y, row in enumerate(rows):
                    high=all(bits[((y//8)*width+x)>>3] & (1<<(((y//8)*width+x)&7)) for x in range(width))
                    self.assertEqual(bool(opaque[y]),bool(high and all(v&15 for v in row)))
            for ty in range(height):
                for tx in range(width):
                    # SGDK adds the enabled base priority, flipping resource bit 7.
                    high=not any(v&128 for row in rows[ty*8:ty*8+8] for v in row[tx*8:tx*8+8])
                    index=ty*width+tx
                    self.assertEqual(bool(bits[index>>3] & (1<<(index&7))),high)
        data=json.loads((ROOT/'src/levels/terrestrial/water/t_water1_collision.json').read_text())
        self.assertEqual(data['spawn'],[8,250]); self.assertEqual(len(data['rows_rle']),31)
        for row in data['rows_rle']:
            for run in row:
                self.assertRegex(run,r'^0x[0-9A-F]{4}$')
                self.assertLess(int(run,16)>>8,27)
            self.assertEqual(sum(int(run,16)&255 for run in row),400)
        self.assertEqual(len(values('palette')),48)

    def test_raster_scroll_and_collision(self):
        with tempfile.TemporaryDirectory() as temp:
            (Path(temp)/'t_water1_data.h').write_text(generate())
            binary=str(Path(temp)/'water-test')
            sources=['tools/tests/water_level_test.c','src/levels/terrestrial/water/t_water_1.c',
                     'src/player.c','src/input_controls.c','src/player_sprite.c','src/camera.c',
                     'src/level_collision/collision.c','src/level_collision/collision_heightmaps.c']
            subprocess.run([os.environ.get('HOST_CC','cc'),'-std=c99','-O2','-Wall','-Wextra','-Werror',
                            '-fsanitize=undefined','-fno-sanitize-recover=undefined',
                            '-I'+temp,'-I'+str(ROOT/'inc'),'-I'+str(ROOT/'src'),
                            *[str(ROOT/s) for s in sources],'-o',binary],check=True)
            subprocess.run([binary],check=True)
