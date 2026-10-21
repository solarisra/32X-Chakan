"""Run the hardware-independent player/input checks with the host C compiler."""
import os
import subprocess
import tempfile
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent


class PlayerTests(unittest.TestCase):
    def test_gameplay_behavior(self):
        with tempfile.TemporaryDirectory() as temp:
            binary = str(Path(temp) / 'player_test')
            subprocess.run([
                os.environ.get('HOST_CC', 'cc'), '-std=c99', '-Wall', '-Wextra',
                '-Werror', '-fsanitize=undefined', '-fno-sanitize-recover=undefined',
                '-I' + str(ROOT / 'inc'), str(ROOT / 'tools/tests/player_test.c'),
                str(ROOT / 'src/player.c'), str(ROOT / 'src/input_controls.c'),
                '-o', binary,
            ], check=True)
            subprocess.run([binary], check=True)


if __name__ == '__main__':
    unittest.main()
