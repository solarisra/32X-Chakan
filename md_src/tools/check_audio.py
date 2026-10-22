#!/usr/bin/env python3
"""Headless PicoDrive smoke check; captures audio without a desktop or device.

python3 md_src/tools/check_audio.py --core /path/to/picodrive_libretro.so
"""
import argparse
import ctypes as C
from pathlib import Path


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--core', required=True)
    parser.add_argument('--rom', default='Chakan.32x')
    parser.add_argument('--frames', type=int, default=7200)
    parser.add_argument('--min-render-fps', type=float, default=0, help='minimum mean rendered frames per 60-frame interval after warmup')
    parser.add_argument('--portal-hub', action='store_true', help='drive Portal Hub and verify SH-2 map bounds')
    parser.add_argument('--portal-entry', choices=('earth','fire','water','air'),
                        help='place Chakan at an entrance and verify Up transitions and music in a Hub ROM')
    parser.add_argument('--return-hub',action='store_true',help='test Start+A+C after --portal-entry, then re-enter the portal')
    parser.add_argument('--logo',action='store_true',help='verify silent logo startup, scream cue, and transition to STATE_INTRO')
    parser.add_argument('--intro',action='store_true',help='run all five intro scenes and verify the automatic menu transition (requires 6700+ frames)')
    parser.add_argument('--fire1', action='store_true', help='drive Fire 1 and verify SH-2 bounds')
    parser.add_argument('--water1', action='store_true', help='drive Water 1 and verify SH-2 bounds')
    parser.add_argument('--air1', action='store_true', help='drive Air 1 and verify SH-2 bounds')
    parser.add_argument('--earth1', action='store_true', help='drive Earth 1 and verify SH-2 world/camera bounds')
    parser.add_argument('--wide', action='store_true', help='drive TEST_WIDE and verify SH-2 frame/camera telemetry')
    args = parser.parse_args()
    if args.intro:
        if args.frames<6700: parser.error('--intro requires at least 6700 frames')
        args.logo=True
    if args.return_hub and (not args.portal_entry or args.frames<1200):
        parser.error('--return-hub requires --portal-entry and at least 1200 frames')
    startup_delay=420 if args.portal_entry else 0
    args.frames+=startup_delay
    if args.portal_hub or args.earth1 or args.air1 or args.water1 or args.fire1 or args.portal_entry or args.logo:
        args.wide = True
    if args.min_render_fps and not args.wide:
        parser.error('--min-render-fps requires --wide')
    if args.frames < 600:
        parser.error('use at least 600 frames to cover startup and sustained playback')
    lib = C.CDLL(str(Path(args.core).resolve()))
    env_type = C.CFUNCTYPE(C.c_bool, C.c_uint, C.c_void_p)
    video_type = C.CFUNCTYPE(None, C.c_void_p, C.c_uint, C.c_uint, C.c_size_t)
    audio_type = C.CFUNCTYPE(None, C.c_int16, C.c_int16)
    batch_type = C.CFUNCTYPE(C.c_size_t, C.POINTER(C.c_int16), C.c_size_t)
    poll_type = C.CFUNCTYPE(None)
    input_type = C.CFUNCTYPE(C.c_int16, C.c_uint, C.c_uint, C.c_uint, C.c_uint)
    stats = {'video': 0, 'samples': 0, 'nonzero': 0, 'peak': 0}
    periods = []
    directory = C.create_string_buffer(str(Path(args.rom).resolve().parent).encode())

    @env_type
    def environment(command, data):
        if command in (9, 31):  # system/save directories
            C.cast(data, C.POINTER(C.c_char_p))[0] = C.cast(directory, C.c_char_p)
            return True
        if command == 17:  # variables unchanged
            C.cast(data, C.POINTER(C.c_bool))[0] = False
            return True
        return command in (10, 11, 16, 18, 35)

    @video_type
    def video(data, width, height, pitch):
        stats['video'] += 1

    def capture(values):
        for value in values:
            stats['samples'] += 1
            stats['nonzero'] += value != 0
            stats['peak'] = max(stats['peak'], abs(value))

    @audio_type
    def audio(left, right):
        capture((left, right))

    @batch_type
    def batch(data, count):
        capture(data[:count * 2])
        return count

    @poll_type
    def poll():
        pass

    @input_type
    def input_state(port, device, index, button):
        # Exercise controller bus requests and movement while music runs.
        frame = stats['video']-startup_delay
        if args.logo: return 0
        if args.portal_entry:
            if args.return_hub and 600<=frame<660:
                return int(port==0 and button in (3,1,8)) # PicoDrive: Start, Genesis A, C
            if args.return_hub and 660<=frame<900:
                return 0
            if args.return_hub and 900<=frame<960:
                return int(port==0 and button==4)
            return int(port==0 and ((button==4 and 240<=frame<300)
                                   or (button==7 and frame>=360)))
        if args.wide:
            return int(port == 0 and (button == 7 or (button == 0 and frame % 60 < 25)))
        return int(port == 0 and ((button == 7 and frame % 600 < 180)
                   or (button == 6 and 300 <= frame % 600 < 480)
                   or (button == 0 and frame % 120 < 15)))

    callbacks = dict(environment=environment, video_refresh=video,
                     audio_sample=audio, audio_sample_batch=batch,
                     input_poll=poll, input_state=input_state)
    for name, callback in callbacks.items():
        getattr(lib, 'retro_set_' + name)(callback)

    class Game(C.Structure):
        _fields_ = [('path', C.c_char_p), ('data', C.c_void_p),
                    ('size', C.c_size_t), ('meta', C.c_char_p)]
    lib.retro_load_game.argtypes = [C.POINTER(Game)]
    lib.retro_load_game.restype = C.c_bool
    rom = Path(args.rom).resolve()
    contents = rom.read_bytes()
    buffer = C.create_string_buffer(contents)
    game = Game(str(rom).encode(), C.cast(buffer, C.c_void_p), len(contents), None)
    lib.retro_init()
    if not lib.retro_load_game(C.byref(game)):
        raise SystemExit('PicoDrive could not load the ROM')
    telemetry = {}
    render_intervals = []
    offsets = {}
    if args.wide:
        for line in (rom.parent / 'md_start.lst').read_text().splitlines():
            fields = line.split()
            if len(fields) == 3 and fields[2] in ('sh2_rendered_frames', 'sh2_camera_x', 'sh2_player_x', 'sh2_profile','sh2_level','sh2_gamestate'):
                offsets[fields[2]] = int(fields[0], 16) & 65535
    lib.retro_get_memory_data.argtypes = [C.c_uint]
    lib.retro_get_memory_data.restype = C.c_void_p

    def read_telemetry():
        pointer = lib.retro_get_memory_data(2)
        if not pointer:
            raise SystemExit('PicoDrive did not expose work RAM')
        ram = C.string_at(pointer, 65536)
        result = {}
        for name, offset in offsets.items():
            # PicoDrive exposes host-order 16-bit RAM words on this host.
            values = [int.from_bytes(ram[offset + i:offset + i + 2], 'little')
                      for i in range(0, 20 if name == 'sh2_profile' else 2, 2)]
            result[name] = values if name == 'sh2_profile' else values[0]
        return result

    # Stable two-second windows within each intro scene, away from load/fade
    # boundaries. Count completed framebuffer presentations, not simulation ticks.
    intro_windows={900:'opening',2900:'story 2',3400:'story 3',3950:'story 4',5100:'hourglass'}
    intro_starts={}
    intro_rates={}
    try:
        previous = 0
        previous_rendered = 0
        for frame in range(args.frames):
            test_frame=frame-startup_delay
            if args.intro and frame in (1000,3000,6000,6600):
                expected=2 if frame==6600 else 1
                if read_telemetry().get('sh2_gamestate')!=expected:
                    raise SystemExit('Intro scene sequence/menu transition failed')
            if args.logo and frame in (100,180,300,500):
                state=read_telemetry().get('sh2_gamestate')
                if frame<500 and state!=0 or frame==500 and state!=1:
                    raise SystemExit('Logo state timing/transition failed')
                if frame==180 and stats['nonzero']:
                    raise SystemExit('Music/audio started before the logo scream cue')
                if frame==300 and not stats['nonzero']:
                    raise SystemExit('Logo scream cue did not produce audio')
            if args.return_hub and test_frame==850:
                if read_telemetry().get('sh2_level')!=0:
                    raise SystemExit('Start+A+C did not restore Portal Hub and its music')
                # Stage Select has no PSG part. Any surviving tone/noise here
                # belongs to the old stage and must have been silenced.
                from picodrive_state import psg_attenuation
                volumes=psg_attenuation(lib)
                print('PSG attenuation after returning to Hub:',volumes)
                if volumes!=[15]*4:
                    raise SystemExit('Old stage PSG channels survived the Hub transition')
            if args.portal_entry and (test_frame==240 or (args.return_hub and test_frame==900)):
                from picodrive_state import place_player
                positions={'earth':(400,192),'fire':(1728,352),
                           'water':(608,1152),'air':(1380,1110)}
                place_player(lib,rom.parent/'Chakan.lst',*positions[args.portal_entry])
            lib.retro_run()
            if args.intro and frame in intro_windows:
                intro_starts[frame]=read_telemetry()['sh2_rendered_frames']
            if args.intro and frame-120 in intro_windows:
                snapshot=read_telemetry()
                if snapshot['sh2_gamestate']!=1:
                    raise SystemExit('Intro performance window fell outside STATE_INTRO')
                start=frame-120
                completed=(snapshot['sh2_rendered_frames']-intro_starts[start])&65535
                intro_rates[intro_windows[start]]=completed/2
            if args.wide and (frame + 1) % 60 == 0:
                telemetry = read_telemetry()
                rendered = telemetry.get('sh2_rendered_frames', 0)
                render_intervals.append((rendered - previous_rendered) & 65535)
                previous_rendered = rendered
            if (frame + 1) % 600 == 0:
                periods.append(stats['nonzero'] - previous)
                previous = stats['nonzero']
        if args.wide:
            telemetry = read_telemetry()
    finally:
        lib.retro_unload_game()
        lib.retro_deinit()
    print(stats)
    print('Nonzero samples per 600-frame period:', periods)
    if args.wide:
        print('SH-2 telemetry:', telemetry)
        if 'sh2_profile' in telemetry:
            stages = ('simulation', 'master draw', 'slave draw', 'master wait', 'flip wait')
            print('Approximate average stage ms (NTSC):', {name: round(ticks * 1000 / (60 * 262), 3) for name, ticks in zip(stages, telemetry['sh2_profile'][:5])})
        steady = render_intervals[1:]
        if steady:
            mean = sum(steady) / len(steady)
            print(f'Rendered frames per 60 VBlanks after warmup: mean={mean:.3f}, min={min(steady)}, max={max(steady)}')
            if not args.intro and mean < args.min_render_fps:
                raise SystemExit('Renderer failed its requested performance threshold')
        if args.intro:
            print('Completed intro presentations per second:',intro_rates)
            if len(intro_rates)!=5 or any(rate<args.min_render_fps for rate in intro_rates.values()):
                raise SystemExit('Intro renderer failed its requested performance threshold')
        if telemetry.get('sh2_rendered_frames', 0) < 100:
            raise SystemExit('Dual-SH-2 renderer did not progress')
        if args.portal_entry:
            expected={'earth':1,'fire':2,'water':3,'air':4}[args.portal_entry]
            if telemetry.get('sh2_level')!=expected:
                raise SystemExit('Portal failed to switch level/music')
        elif args.earth1 or args.air1 or args.water1 or args.fire1:
            if not (0 <= telemetry.get('sh2_camera_x', -1) <= (1280 if args.fire1 else 6080 if args.water1 else 1600 if args.air1 else 2256) and
                    0 <= telemetry.get('sh2_player_x', -1) <= (1558 if args.fire1 else 6358 if args.water1 else 1878 if args.air1 else 2534)):
                raise SystemExit('SH-2 player/camera escaped world bounds')
        elif not args.logo and args.frames >= 1200 and (telemetry.get('sh2_camera_x') != (1600 if args.portal_hub else 1120) or telemetry.get('sh2_player_x') != (1878 if args.portal_hub else 1398)):
            raise SystemExit('SH-2 level did not reach the rightmost world/camera bounds')
    if stats['video'] != args.frames or not periods or not all(periods):
        raise SystemExit('Audio/video smoke check failed')


if __name__ == '__main__':
    main()
