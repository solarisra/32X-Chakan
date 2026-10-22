"""Test-only placement in PicoDrive save states; no debug commands in the ROM.

PicoDrive state.c stores chunk 36 as 256 KiB of little-endian uint16_t SDRAM.
Use the build's symbol table so struct placement is never a fixed ROM address.
"""
import ctypes as C
import struct


def psg_attenuation(core):
    """Read actual tone/noise volume registers from PicoDrive's PSG chunk."""
    core.retro_serialize_size.restype=C.c_size_t
    core.retro_serialize.argtypes=[C.c_void_p,C.c_size_t]
    core.retro_serialize.restype=C.c_bool
    size=core.retro_serialize_size()
    state=C.create_string_buffer(size)
    if not core.retro_serialize(state,size):
        raise RuntimeError('Could not inspect PicoDrive PSG state')
    data=state.raw
    if data[:8]!=b'PicoSEXT':
        raise RuntimeError('Unrecognized PicoDrive state format')
    offset=12
    while offset+5<len(data):
        kind=data[offset]
        count=struct.unpack_from('<I',data,offset+1)[0]
        offset+=5
        if offset+count>len(data):
            break
        if kind==10 and count==28*4:
            registers=struct.unpack_from('<28I',data,offset)
            return [registers[i]&15 for i in (1,3,5,7)]
        offset+=count
    raise RuntimeError('PicoDrive PSG chunk missing')


def place_player(core, symbols, x, y):
    address=None
    for line in symbols.read_text().splitlines():
        fields=line.split()
        if len(fields)==3 and fields[2]=='_playerBody':
            address=int(fields[0],16)-0x06000000
    if address is None or not 0<=address<0x40000-24:
        raise RuntimeError('SH-2 player symbol missing or outside SDRAM')
    core.retro_serialize_size.restype=C.c_size_t
    size=core.retro_serialize_size()
    state=C.create_string_buffer(size)
    for name in ('retro_serialize','retro_unserialize'):
        fn=getattr(core,name)
        fn.argtypes=[C.c_void_p,C.c_size_t]
        fn.restype=C.c_bool
    if not core.retro_serialize(state,size):
        raise RuntimeError('Could not save PicoDrive state')
    data=bytearray(state.raw)
    if data[:8]!=b'PicoSEXT':
        raise RuntimeError('Unrecognized PicoDrive state format')
    offset=12
    while offset+5<len(data):
        kind=data[offset]
        count=struct.unpack_from('<I',data,offset+1)[0]
        offset+=5
        if offset+count>len(data):
            break
        if kind==36 and count==0x40000:
            position=offset+address
            struct.pack_into('<9h',data,position,x,y,0,0,0,0,0,58,1)
            data[position+18:position+24]=bytes(6)
            C.memmove(state,bytes(data),size)
            if not core.retro_unserialize(state,size):
                raise RuntimeError('Could not restore PicoDrive state')
            return
        offset+=count
    raise RuntimeError('PicoDrive SDRAM chunk missing')
