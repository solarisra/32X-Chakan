"""Reconstruct every packed intro plane/sprite against its original pixels."""
import re
import unittest
from indexed_png import read_png
from convert_intro import ROOT,generate,scene_pixels

class IntroAssetsTests(unittest.TestCase):
    def test_planes_and_six_animation_frames(self):
        text=generate()
        def values(name):
            match=re.search(r'intro_'+name+r'\[\] = \{(.*?)\};',text,re.S)
            return [int(v) for v in re.findall(r'\d+',match[1])]
        for number in range(1,6):
            for plane in ('fg','bg'):
                w,h,rows,_=scene_pixels(number,plane)
                name=f'{number}_{plane}'
                tiles,offsets,tilemap=[values(name+'_'+part) for part in ('tiles','rows','map')]
                stride=5 if plane=='fg' else 2
                width=40 if plane=='fg' else 64
                for y,row in enumerate(rows):
                    decoded=[]
                    for x in range(w):
                        tile=tilemap[(y//8)*width+x//8]
                        packed=tiles[offsets[tile+y%8]+(x%8)//4]
                        decoded.append((packed>>((3-x%4)*8))&255)
                    base=16 if number==5 and plane=='fg' else 0
                    self.assertEqual(decoded,[v+base if v else 0 for v in row])
                self.assertTrue(all(v%stride==0 for v in offsets))
        _,_,background,_=scene_pixels(1,'bg')
        indices={v for row in background for v in row}
        self.assertTrue(set(range(33,47))<=indices)
        self.assertEqual(scene_pixels(1,'bg')[:2],(320,224))
        _,_,source,_=read_png(ROOT/'res/intro/intro_sprite_indexed.png')
        offsets,spans,pixels=[values('chakan_'+part) for part in ('rows','spans','pixels')]
        for flip in range(2):
            for frame in range(6):
                for y in range(88):
                    decoded=[0]*112
                    row=(flip*6+frame)*88+y
                    for span in range(offsets[row],offsets[row+1]):
                        x,count,start=spans[span*3:span*3+3]
                        decoded[x:x+count]=pixels[start:start+count]
                    expected=source[y][frame*112:(frame+1)*112]
                    if flip: expected=expected[::-1]
                    self.assertEqual(decoded,[v+16 if v else 0 for v in expected])
