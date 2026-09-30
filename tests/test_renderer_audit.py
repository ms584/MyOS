"""Static renderer inventory tests, not native rendering/Cobalt acceptance."""
from pathlib import Path
import struct
import sys
import tempfile
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
from measure_renderer import load_segments, table_slots

class InventoryTests(unittest.TestCase):
    def test_order_offsets_comments(self):
        source = '''typedef struct Test {
            // void (*glFake)(void);
            void ( * glFirst )(int x);
            /* comment */ const char *(*glSecond)(void);
        } Test;'''
        self.assertEqual(table_slots(source, 'Test'),
            [{'name': 'glFirst', 'offset': 0}, {'name': 'glSecond', 'offset': 8}])

    def test_ambiguous_tables_reject(self):
        for members in ['int value;', 'void (*glA)(void); int value;',
                        'void (*glA)(void); void (*glA)(void);']:
            with self.subTest(members=members), self.assertRaises(ValueError):
                table_slots('typedef struct Test {' + members + '} Test;', 'Test')

    def test_elf_tls_preserved(self):
        header = struct.pack('<16sHHIQQQIHHHHHH', b'\x7fELF\x02\x01\x01' + bytes(9),
            3, 62, 1, 0, 64, 0, 0, 64, 56, 2, 0, 0, 0)
        data = header + struct.pack('<IIQQQQQQ', 1, 5, 0, 0x1000, 0, 64, 4096, 4096)
        data += struct.pack('<IIQQQQQQ', 7, 4, 128, 0x2000, 0, 8, 32, 8)
        with tempfile.TemporaryDirectory() as directory:
            library = Path(directory) / 'test.so'; library.write_bytes(data)
            segments = load_segments(library)
            self.assertEqual([s['kind'] for s in segments], ['LOAD', 'TLS'])
            self.assertEqual(segments[1]['memory_bytes'], 32)
            library.write_bytes(data[:-1])
            with self.assertRaises(ValueError): load_segments(library)
            library.write_bytes(b'not ELF')
            with self.assertRaises(ValueError): load_segments(library)

if __name__ == '__main__': unittest.main()
