"""Convert the scream to 75% speed, signed 8-bit 14kHz XGM1 PCM."""
from pathlib import Path
import struct
import wave
ROOT=Path(__file__).resolve().parent.parent
PLAYBACK_RATE=14000
SPEED_NUMERATOR=3
SPEED_DENOMINATOR=4
def generate(source=None, speed_numerator=SPEED_NUMERATOR):
    with wave.open(str(source or ROOT/'res/sfx/chakan_scream2.wav'),'rb') as wav:
        if wav.getsampwidth()!=2 or wav.getcomptype()!='NONE':
            raise ValueError('Expected uncompressed 16-bit source scream')
        channels,rate,frames=wav.getnchannels(),wav.getframerate(),wav.getnframes()
        samples=struct.unpack('<'+'h'*(frames*channels),wav.readframes(frames))
    mono=[sum(samples[i:i+channels])//channels for i in range(0,len(samples),channels)]
    pcm=bytearray()
    # Stretch the sample itself; the XGM1 driver still plays at 14kHz.
    denominator=PLAYBACK_RATE*SPEED_DENOMINATOR
    step=rate*speed_numerator
    for i in range((frames*denominator+step-1)//step):
        position,remainder=divmod(i*step,denominator)
        a=mono[min(position,frames-1)]; b=mono[min(position+1,frames-1)]
        value=(a*(denominator-remainder)+b*remainder)//denominator
        pcm.append(max(-128,min(127,value//256))&255)
    pcm.extend(bytes((-len(pcm))%256))
    return bytes(pcm)
if __name__=='__main__': (ROOT/'res/logo_scream.bin').write_bytes(generate())
