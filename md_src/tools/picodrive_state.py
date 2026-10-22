"""Test-only placement in PicoDrive save states; no debug commands in the ROM.

PicoDrive state.c stores chunk 36 as 256 KiB of little-endian uint16_t SDRAM.
Use the build's symbol table so struct placement is never a fixed ROM address.
"""
import ctypes as C
import struct


def xgm_playback(core, saved_symbols=None):
    """Inspect the shipped XGM driver's status, position and elapsed ticks."""
    if saved_symbols is not None:
        for line in saved_symbols.read_text().splitlines():
            fields=line.split()
            if len(fields)==3 and fields[2]=='saved_driver':
                address=int(fields[0],16)&65535
                ram=C.string_at(core.retro_get_memory_data(2),65536)
                # PicoDrive work RAM exposes host-order 68000 words.
                raw=ram[address:address+8192]
                zram=bytearray(8192)
                for i in range(8192): zram[i]=raw[i^1]
                return (bool(zram[0x102]&0x40),
                        int.from_bytes(zram[0x194:0x197],'little'),
                        int.from_bytes(zram[0x134:0x137],'little'))
        raise RuntimeError('Saved music driver symbol missing')
    core.retro_serialize_size.restype=C.c_size_t
    core.retro_serialize.argtypes=[C.c_void_p,C.c_size_t]
    core.retro_serialize.restype=C.c_bool
    size=core.retro_serialize_size()
    state=C.create_string_buffer(size)
    if not core.retro_serialize(state,size):
        raise RuntimeError('Could not inspect PicoDrive XGM state')
    data=state.raw
    offset=12
    while offset+5<len(data):
        kind=data[offset]
        count=struct.unpack_from('<I',data,offset+1)[0]
        offset+=5
        if kind==4 and count==8192:
            ram=data[offset:offset+count]
            return (bool(ram[0x102]&0x40),
                    int.from_bytes(ram[0x194:0x197],'little'),
                    int.from_bytes(ram[0x134:0x137],'little'))
        offset+=count
    raise RuntimeError('PicoDrive Z80 RAM chunk missing')


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


def verify_music_instruments(core, symbols):
    """Compare the host's write-only FM register shadow with the emulated chip."""
    for line in symbols.read_text().splitlines():
        fields=line.split()
        if len(fields)==3 and fields[2]=='registers':
            address=int(fields[0],16)&65535
            ram=C.string_at(core.retro_get_memory_data(2),65536)
            raw=ram[address:address+552]
            shadow=bytes(raw[i^1] for i in range(len(raw)))
            core.YM2612GetRegs.restype=C.c_void_p
            actual=C.string_at(core.YM2612GetRegs(),512)
            differences=[]
            for bank in range(2):
                for reg in list(range(0x30,0xA0))+list(range(0xB0,0xB7)):
                    if reg&3==3: continue
                    offset=bank*256+reg
                    if shadow[12+offset]!=actual[offset]:
                        differences.append((bank,hex(reg),shadow[12+offset],actual[offset]))
            if differences:
                raise RuntimeError('FM instrument restore/shadow mismatch: '+repr(differences[:8])+' state='+shadow[:12].hex())
            return
    raise RuntimeError('Music register shadow symbol missing')
