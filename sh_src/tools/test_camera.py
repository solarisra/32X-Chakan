"""Camera controls, lookahead, cinematics, safe shake and scrolling adapters."""
import os
from pathlib import Path
import subprocess
import tempfile
import unittest
ROOT=Path(__file__).resolve().parent.parent
class CameraTests(unittest.TestCase):
    def test_camera_controls(self):
        with tempfile.TemporaryDirectory() as temp:
            binary=str(Path(temp)/'camera')
            subprocess.run([os.environ.get('HOST_CC','cc'),'-std=c99','-O2','-Wall','-Wextra','-Werror',
                '-fsanitize=undefined','-fno-sanitize-recover=undefined','-I'+str(ROOT/'inc'),
                str(ROOT/'tools/tests/camera_test.c'),str(ROOT/'src/camera.c'),'-o',binary],check=True)
            subprocess.run([binary],check=True)
