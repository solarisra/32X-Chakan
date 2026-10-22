"""Progression ordering, realm unlocking, invalid IDs and reset behavior."""
import os
from pathlib import Path
import subprocess
import tempfile
import unittest
ROOT=Path(__file__).resolve().parent.parent
class LevelProgressTests(unittest.TestCase):
    def test_progression(self):
        with tempfile.TemporaryDirectory() as temp:
            binary=str(Path(temp)/'progress')
            subprocess.run([os.environ.get('HOST_CC','cc'),'-std=c99','-O2','-Wall','-Wextra','-Werror',
                '-fsanitize=undefined','-fno-sanitize-recover=undefined','-I'+str(ROOT/'inc'),
                str(ROOT/'tools/tests/level_progress_test.c'),str(ROOT/'src/level_progress.c'),'-o',binary],check=True)
            subprocess.run([binary],check=True)
