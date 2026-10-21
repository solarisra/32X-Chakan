"""Run the extracted, hardware-independent GDL rasterizer on the host."""
import os
import subprocess
import tempfile
import unittest
from pathlib import Path
ROOT = Path(__file__).resolve().parent.parent


class GdlRendererTests(unittest.TestCase):
    def test_renderer(self):
        with tempfile.TemporaryDirectory() as temp:
            binary = str(Path(temp) / 'renderer-test')
            subprocess.run([os.environ.get('HOST_CC', 'cc'), '-std=c99', '-O2',
                            '-Wall', '-Wextra', '-Werror', '-fsanitize=undefined',
                            '-fno-sanitize-recover=undefined', '-I' + str(ROOT / 'inc'),
                            str(ROOT / 'tools/tests/renderer_test.c'),
                            str(ROOT / 'src/gdl/renderer.c'), str(ROOT / 'src/gdl/renderer_direct.c'), '-o', binary], check=True)
            subprocess.run([binary], check=True)


if __name__ == '__main__':
    unittest.main()
