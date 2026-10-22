"""Preserve the original 4bpp pause window for the 32X screen renderer."""
import struct
from pathlib import Path
from gdl_assets import native_palette
ROOT=Path(__file__).resolve().parent.parent
def generate():
    data=(ROOT/'res/menus/pause_menu.bmp').read_bytes()
    offset,dib=struct.unpack_from('<I',data,10)[0],struct.unpack_from('<I',data,14)[0]
    w,h=struct.unpack_from('<ii',data,18)
    depth,compression=struct.unpack_from('<HI',data,28)
    if (w,h,depth,compression)!=(320,224,4,0): raise ValueError('Expected 320x224 4bpp pause BMP')
    palette=data[14+dib:offset]
    colors=[((palette[i]>>5)<<9)|((palette[i+1]>>5)<<5)|((palette[i+2]>>5)<<1) for i in range(0,len(palette),4)]
    colors=native_palette(colors+[0]*(16-len(colors)))
    rows=[];stride=((w*4+31)//32)*4
    for y in range(h):
        row=data[offset+(h-1-y)*stride:offset+(h-y)*stride]
        rows.append([80+((row[x//2]>>(4 if x%2==0 else 0))&15) for x in range(w)])
    def words(rows):return [sum(row[x+i]<<(24-i*8) for i in range(4)) for row in rows for x in range(0,len(row),4)]
    out=[]
    def array(kind,name,values):
        out.append(f'static const {kind} pause_{name}[] = {{')
        for i in range(0,len(values),20):out.append(','.join(map(str,values[i:i+20]))+',')
        out.append('};')
    array('uint32_t','screen',words(rows));array('uint16_t','rows',[0]*225)
    array('uint32_t','patches',[0]);array('uint16_t','palette',colors)
    return '\n'.join(out)+'\n'
if __name__=='__main__':(ROOT/'res/pause_data.h').write_text(generate())
