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
    parser.add_argument('--wide', action='store_true', help='drive TEST_WIDE and verify SH-2 frame/camera telemetry')
    args = parser.parse_args()
    if args.portal_hub:
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
        frame = stats['video']
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
            if len(fields) == 3 and fields[2] in ('sh2_rendered_frames', 'sh2_camera_x', 'sh2_player_x', 'sh2_profile'):
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

    try:
        previous = 0
        previous_rendered = 0
        for frame in range(args.frames):
            lib.retro_run()
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
            if mean < args.min_render_fps:
                raise SystemExit('Renderer failed its requested performance threshold')
        if telemetry.get('sh2_rendered_frames', 0) < 100:
            raise SystemExit('Dual-SH-2 renderer did not progress')
        if args.frames >= 1200 and (telemetry.get('sh2_camera_x') != (1600 if args.portal_hub else 1120) or telemetry.get('sh2_player_x') != (1878 if args.portal_hub else 1398)):
            raise SystemExit('SH-2 level did not reach the rightmost world/camera bounds')
    if stats['video'] != args.frames or not periods or not all(periods):
        raise SystemExit('Audio/video smoke check failed')


if __name__ == '__main__':
    main()
