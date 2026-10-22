"""Run actual Hub entrance selection and all five level loaders on the host."""
import os
import subprocess
import tempfile
import unittest
from pathlib import Path
from convert_portal_hub import generate as hub
from convert_t_earth1 import generate as earth
from convert_t_fire1 import generate as fire
from convert_t_water1 import generate as water
from convert_t_air1 import generate as air
ROOT=Path(__file__).resolve().parent.parent


class PortalTransitionTests(unittest.TestCase):
    def test_entrances_and_runtime_loaders(self):
        with tempfile.TemporaryDirectory() as temp:
            for name,generate in [('portal_hub',hub),('t_earth1',earth),('t_fire1',fire),
                                  ('t_water1',water),('t_air1',air)]:
                (Path(temp)/(name+'_data.h')).write_text(generate())
            sources=['tools/tests/portal_transition_test.c','src/level_manager.c','src/level_progress.c',
                     'src/levels/hub/portal_hub.c','src/levels/hub/hub_sprites.c',
                     'src/levels/hub/planets.c',
                     'src/levels/terrestrial/earth_levels.c',
                     'src/levels/terrestrial/fire/t_fire_1.c',
                     'src/levels/terrestrial/water/t_water_1.c',
                     'src/levels/terrestrial/air/t_air_1.c',
                     'src/player.c','src/input_controls.c','src/player_sprite.c','src/camera.c',
                     'src/level_collision/collision.c','src/level_collision/collision_heightmaps.c']
            binary=str(Path(temp)/'portals-test')
            subprocess.run([os.environ.get('HOST_CC','cc'),'-std=c99','-O2','-Wall','-Wextra','-Werror',
                            '-fsanitize=undefined','-fno-sanitize-recover=undefined',
                            '-I'+temp,'-I'+str(ROOT/'inc'),'-I'+str(ROOT/'src'),
                            *[str(ROOT/s) for s in sources],'-o',binary],check=True)
            subprocess.run([binary],check=True)
