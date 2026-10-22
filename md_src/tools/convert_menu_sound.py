"""Separate the prerecorded menu sequence from individually triggered title hits."""
from pathlib import Path
import struct
import wave
from convert_logo_sound import PLAYBACK_RATE
ROOT=Path(__file__).resolve().parent.parent
# The source WAV bundles the title impacts into its lightning sequence.
TITLE_START=4.65
TITLE_END=6.90
IMPACT_START=4.76
IMPACT_END=5.05

def generate():
    with wave.open(str(ROOT/'res/sfx/menu_lightning.wav'),'rb') as wav:
        if wav.getsampwidth()!=2 or wav.getcomptype()!='NONE':
            raise ValueError('Expected uncompressed 16-bit menu sound')
        channels,rate,frames=wav.getnchannels(),wav.getframerate(),wav.getnframes()
        samples=struct.unpack('<'+'h'*(frames*channels),wav.readframes(frames))
    mono=[sum(samples[i:i+channels])//channels for i in range(0,len(samples),channels)]
    def encode(start,end,mute_title=False):
        pcm=bytearray()
        for i in range(round((end-start)*PLAYBACK_RATE)):
            t=start+i/PLAYBACK_RATE
            position=min(int(t*rate),frames-1)
            value=mono[position]
            if mute_title and TITLE_START<=t<TITLE_END: value=0
            # Short edge ramps avoid clicks at the extracted impact boundaries.
            if not mute_title:
                gain=min(1,i/70,(round((end-start)*PLAYBACK_RATE)-1-i)/140)
                value=int(value*gain)
            pcm.append(max(-128,min(127,value//256))&255)
        pcm.extend(bytes((-len(pcm))%256))
        return bytes(pcm)
    return encode(0,frames/rate,True),encode(IMPACT_START,IMPACT_END)

if __name__=='__main__':
    lightning,letter=generate()
    (ROOT/'res/menu_lightning.bin').write_bytes(lightning)
    (ROOT/'res/menu_letter.bin').write_bytes(letter)
