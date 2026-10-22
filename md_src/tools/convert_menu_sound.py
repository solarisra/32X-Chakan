"""Preserve the complete menu recording, including lightning and title impacts."""
from pathlib import Path
from convert_logo_sound import generate as convert_pcm
ROOT=Path(__file__).resolve().parent.parent

def generate():
    # One continuous recording keeps thunder tails and overlapping impacts.
    # Match the visuals to it; muting its title interval also removes lightning.
    return convert_pcm(ROOT/'res/sfx/menu_lightning.wav',4)

if __name__=='__main__':
    (ROOT/'res/menu_lightning.bin').write_bytes(generate())
