"""Select a CPU/rendering backend without stale objects on mode switches."""
from pathlib import Path
import sys
names = ('TEST_WIDE', 'TEST_SMALL', 'PORTAL_HUB', 'T_EARTH_1', 'T_AIR_1', 'T_WATER_1', 'T_FIRE_1')
if sys.argv[1] not in names:
    raise SystemExit('GAME must be T_FIRE_1, T_WATER_1, T_AIR_1, T_EARTH_1, PORTAL_HUB, TEST_WIDE or TEST_SMALL')
p = Path('build/game_config.h')
p.parent.mkdir(exist_ok=True)
text = ''.join('#define GAME_' + name + ' ' + str(int(sys.argv[1] == name)) + '\n' for name in names)
text += '#define GAME_SH2 (GAME_TEST_WIDE || GAME_PORTAL_HUB || GAME_T_EARTH_1 || GAME_T_AIR_1 || GAME_T_WATER_1 || GAME_T_FIRE_1)\n'
if not p.exists() or p.read_text() != text: p.write_text(text)
