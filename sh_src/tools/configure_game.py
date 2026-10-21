"""Select a CPU/rendering backend without leaving stale objects when switching."""
from pathlib import Path
import sys
if sys.argv[1] not in ('TEST_WIDE', 'TEST_SMALL'):
    raise SystemExit('GAME must be TEST_WIDE or TEST_SMALL')
p = Path('build/game_config.h')
p.parent.mkdir(exist_ok=True)
text = '#define GAME_TEST_WIDE ' + str(int(sys.argv[1] == 'TEST_WIDE')) + '\n'
if not p.exists() or p.read_text() != text:
    p.write_text(text)
