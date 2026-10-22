"""Menu conversion preserves the lightning underneath embedded title impacts."""
import unittest
from convert_menu_sound import generate,ROOT
from convert_logo_sound import generate as convert_pcm,PLAYBACK_RATE
class MenuSoundTests(unittest.TestCase):
    def test_complete_recording_preserved(self):
        pcm=generate()
        self.assertEqual(pcm,convert_pcm(ROOT/'res/sfx/menu_lightning.wav',4))
        self.assertEqual(len(pcm)%256,0)
        # The old synchronization edit silenced this whole interval.
        interval=pcm[round(4.65*PLAYBACK_RATE):round(6.9*PLAYBACK_RATE)]
        self.assertGreater(sum(v!=0 for v in interval),len(interval)//2)
