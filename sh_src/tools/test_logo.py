"""Logo asset reconstruction and actual state timing, under UB sanitization."""
import os
import re
import subprocess
import tempfile
import unittest
from pathlib import Path
from convert_logo import generate
from convert_menu import generate as generate_menu
from convert_intro import generate as generate_intro
from indexed_png import read_png
ROOT=Path(__file__).resolve().parent.parent
class LogoTests(unittest.TestCase):
    def test_menu_embedded_palette_and_header(self):
        for name in ('lightning_a','lightning_b','splash_lettering','lettering'):
            w,h,rows,palette=read_png(ROOT/'res/menus'/('main_menu_'+name+'.png'),
                                      rgb_palette=[],embedded_palette=True)
            self.assertEqual((w,h),(320,224))
            self.assertEqual(palette[5],0xAA) # 3-bit red/green yellow placeholder, slot 5
            if name=='lightning_a':
                self.assertEqual(rows[32][24],1)
                self.assertEqual(rows[40][248],5)
        generated=generate_menu()
        self.assertIn('#define MENU_FAR_H 28',generated)
        self.assertIn('#define MENU_CLOSE_H 28',generated)

    def test_source_pixels_and_state_timing(self):
        text=generate()
        def values(name):
            return [int(v) for v in re.findall(r'\d+',re.search(r'logo_'+name+r'\[\] = \{(.*?)\};',text,re.S)[1])]
        offsets,spans,pixels=values('rows'),values('spans'),values('pixels')
        _,_,source,_=read_png(ROOT/'res/sgdk_retro_logo.png')
        for flip in range(2):
            for y in range(32):
                row=[0]*96; index=flip*32+y
                for span in range(offsets[index],offsets[index+1]):
                    x,count,position=spans[span*3:span*3+3]
                    row[x:x+count]=pixels[position:position+count]
                self.assertEqual(row,source[y][::-1] if flip else source[y])
        with tempfile.TemporaryDirectory() as temp:
            (Path(temp)/'logo_data.h').write_text(text)
            (Path(temp)/'menu_data.h').write_text(generate_menu())
            (Path(temp)/'intro_data.h').write_text(generate_intro())
            binary=str(Path(temp)/'logo-test')
            files=['tools/tests/logo_state_test.c','src/gamestate.c','src/gamestates/gamestate_logo.c','src/gamestates/gamestate_menu.c','src/gamestates/gamestate_intro.c',
                   'src/gdl/renderer.c','src/gdl/renderer_direct.c','src/gdl/renderer_planes.c']
            subprocess.run([os.environ.get('HOST_CC','cc'),'-std=c99','-O2','-Wall','-Wextra','-Werror',
                            '-fsanitize=undefined','-fno-sanitize-recover=undefined',
                            '-I'+temp,'-I'+str(ROOT/'inc'),
                            *[str(ROOT/f) for f in files],'-o',binary],check=True)
            subprocess.run([binary],check=True)
