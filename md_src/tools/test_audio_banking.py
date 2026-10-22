"""Check the actual XGM bank macros' serialized addresses and cycle budget."""
import re
import unittest
from pathlib import Path


class AudioBankingTests(unittest.TestCase):
    def test_mirror_and_pcm_timing(self):
        source = (Path(__file__).resolve().parents[1] / 'inc/snd/z80_mac.i80').read_text()
        for register, expected_cycles in [('BC', 101), ('DE', 105)]:
            body = re.search(r'macro\s+setBank_' + register + r'\s*\n(.*?)\bendm', source, re.S)[1]
            for address in range(0x880000, 0x900000, 0x8000):
                with self.subTest(register=register, address=hex(address)):
                    # The callers rotate address bits 15..22 into A before banking.
                    accumulator = (address >> 15) & 255
                    writes, cycles = [], 0
                    for raw in body.splitlines():
                        instruction = ' '.join(raw.split(';')[0].split())
                        if not instruction:
                            continue
                        if instruction == f'LD {register}, BANKREG':
                            cycles += 10
                        elif instruction == f'LD ({register}), A':
                            writes.append(accumulator & 1)
                            cycles += 7
                        elif instruction == 'RRCA':
                            accumulator = (accumulator >> 1) | ((accumulator & 1) << 7)
                            cycles += 4
                        elif instruction == 'XOR A':
                            accumulator = 0
                            cycles += 4
                        elif instruction == 'CPL':
                            accumulator ^= 255
                            cycles += 4
                        elif instruction == 'NOP':
                            cycles += 4
                        else:
                            self.fail(f'Unhandled bank instruction: {instruction}')
                    self.assertEqual(len(writes), 9)
                    bank = sum(bit << index for index, bit in enumerate(writes))
                    self.assertEqual(bank << 15, address)
                    self.assertEqual(cycles, expected_cycles)
