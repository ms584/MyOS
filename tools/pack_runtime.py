#!/usr/bin/env python3
"""Deterministic MRT1 runtime packaging, not media compression.

16-byte little-endian header: magic, raw length, payload length, raw CRC32.
Tokens: 0..127 literal length minus one; 128..255 match length minus three
plus bit 7, followed by a nonzero 16-bit backward distance. Overlap allowed.
The loader independently limits and verifies the raw length before mapping RX.
"""
import argparse
from pathlib import Path
import struct
import zlib


def pack(raw):
    if not 0 < len(raw) <= 65536:
        raise ValueError('runtime must contain 1..65536 bytes')
    packed = bytearray()
    literal = bytearray()
    seen = {}
    pos = 0

    def flush():
        while literal:
            count = min(len(literal), 128)
            packed.append(count - 1)
            packed.extend(literal[:count])
            del literal[:count]

    def remember(index):
        chain = seen.setdefault(raw[index:index + 3], [])
        chain.append(index)
        if len(chain) > 32:
            del chain[0]

    while pos < len(raw):
        key = raw[pos:pos + 3]
        previous, count = -1, 0
        for candidate in reversed(seen.get(key, [])):
            if len(key) != 3 or pos - candidate > 65535:
                continue
            length = 0
            while (length < 130 and pos + length < len(raw)
                   and raw[candidate + length] == raw[pos + length]):
                length += 1
            if length > count:
                count, previous = length, candidate
            if count == 130:
                break
        if count >= 4:
            flush()
            packed.append(128 + count - 3)
            packed.extend(struct.pack('<H', pos - previous))
            for index in range(pos, pos + count):
                remember(index)
            pos += count
        else:
            literal.append(raw[pos])
            remember(pos)
            pos += 1
            if len(literal) == 128:
                flush()
    flush()
    return struct.pack('<4sIII', b'MRT1', len(raw), len(packed),
                       zlib.crc32(raw)) + packed


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('source', type=Path)
    parser.add_argument('destination', type=Path)
    args = parser.parse_args()
    raw = args.source.read_bytes()
    packed = pack(raw)
    args.destination.write_bytes(packed)
    print(f'Runtime: {len(raw)} raw bytes -> {len(packed)} MRT1 bytes')


if __name__ == '__main__':
    main()
