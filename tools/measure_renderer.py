#!/usr/bin/env python3
"""Inventory a HOST renderer candidate. This never proves a native MyOS gate."""
import argparse
from collections import Counter
import hashlib
import json
from pathlib import Path
import re
import struct
import subprocess

ROOT = Path(__file__).resolve().parents[1]
MESA_SHA256 = '592272df3cf01e85e7db300c449df5061092574d099da275d19e97ef0510f8a6'

def table_slots(source, name):
    source = re.sub(r'/\*.*?\*/|//[^\n]*', '', source, flags=re.S)
    match = re.search(r'typedef\s+struct\s+' + re.escape(name) + r'\s*\{(.*?)\}\s*' + re.escape(name) + r'\s*;', source, re.S)
    if not match:
        raise ValueError('missing table: ' + name)
    body = match[1]
    slots = re.findall(r'\(\s*\*\s*((?:egl|gl)\w+)\s*\)\s*\(', body)
    if (not slots or len(slots) != len(set(slots))
            or len(slots) != len(re.findall(r'\(\s*\*', body))
            or len(slots) != body.count(';')):
        raise ValueError('ambiguous/duplicate/non-function table member: ' + name)
    return [{'name': slot, 'offset': i * 8} for i, slot in enumerate(slots)]

def load_segments(library):
    with library.open('rb') as stream:
        header = stream.read(64)
        if len(header) != 64 or header[:7] != b'\x7fELF\x02\x01\x01':
            raise ValueError('expected ELF64 little endian')
        fields = struct.unpack('<16sHHIQQQIHHHHHH', header)
        if fields[2] != 62 or fields[9] != 56 or not 0 < fields[10] <= 64:
            raise ValueError('expected bounded x86-64 program headers')
        segments = []
        for index in range(fields[10]):
            stream.seek(fields[5] + index * 56)
            raw = stream.read(56)
            if len(raw) != 56:
                raise ValueError('truncated program headers')
            kind, flags, offset, address, _, filesz, memsz, alignment = struct.unpack('<IIQQQQQQ', raw)
            if kind in (1, 7):
                segments.append(dict(kind='LOAD' if kind == 1 else 'TLS', flags=flags,
                    offset=offset, address=address, file_bytes=filesz, memory_bytes=memsz,
                    alignment=alignment))
    return segments

def measure(library, headers, archive=None):
    manifest = json.loads((ROOT / 'docs/PHASE331_ABI_SOURCES.json').read_text())
    pinned = {Path(row['path']).name: row['sha256'] for row in manifest['headers']}
    tables = {}
    for filename, name in [('egl.h', 'SbEglInterface'), ('gles.h', 'SbGlesInterface')]:
        data = (headers / filename).read_bytes()
        if hashlib.sha256(data).hexdigest() != pinned[filename]:
            raise ValueError('pinned header mismatch: ' + filename)
        slots = table_slots(data.decode(), name)
        tables[name] = dict(bytes=len(slots)*8, alignment=8, slots=slots)
    archive_hash = None
    if archive:
        with archive.open('rb') as stream:
            archive_hash = hashlib.file_digest(stream, 'sha256').hexdigest()
        if archive_hash != MESA_SHA256:
            raise ValueError('Mesa 25.0.7 source archive hash mismatch')
    dynamic = subprocess.check_output(['readelf', '-dW', str(library)], text=True)
    symbols = subprocess.check_output(['readelf', '--dyn-syms', '-W', str(library)], text=True)
    relocations = subprocess.check_output(['readelf', '-rW', str(library)], text=True)
    undefined = []
    for line in symbols.splitlines():
        parts = line.split()
        if len(parts) >= 8 and parts[6] == 'UND':
            undefined.append(dict(symbol=parts[7], binding=parts[4]))
    segments = load_segments(library)
    loads = [s for s in segments if s['kind'] == 'LOAD']
    return dict(evidence_kind='HOST_BUILD_STATIC_INVENTORY', native_gate_proven=False,
        backend_selected=False, source_archive_sha256=archive_hash,
        library=str(library), library_bytes=library.stat().st_size,
        load_memory_bytes=sum(s['memory_bytes'] for s in loads),
        address_span_bytes=max(s['address']+s['memory_bytes'] for s in loads)-min(s['address'] for s in loads),
        segments=segments, needed=re.findall(r'\(NEEDED\).*?\[([^]]+)\]', dynamic),
        undefined=undefined, relocation_types=dict(Counter(re.findall(r'\bR_X86_64_\w+', relocations))),
        pinned_cobalt_source=manifest['source_commit'], x64_tables=tables,
        required_native_checks=['context/thread ownership', 'present real framebuffer',
            'GLES shaders/textures/text', 'guard/cleanup/relaunch', 'measured RAM/CPU',
            'native Cobalt HTML/JS/DOM/input/verified HTTPS >=60s'])

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--library', required=True, type=Path)
    parser.add_argument('--headers', required=True, type=Path)
    parser.add_argument('--source-archive', type=Path)
    parser.add_argument('--output', required=True, type=Path)
    args = parser.parse_args()
    try:
        report = measure(args.library, args.headers, args.source_archive)
        args.output.write_text(json.dumps(report, indent=2) + '\n')
        print('HOST inventory only; native gate NOT proven:', args.output)
        print('Tables:', {k: len(v['slots']) for k, v in report['x64_tables'].items()})
        print('Bytes/file/load/span:', report['library_bytes'], report['load_memory_bytes'], report['address_span_bytes'])
        print('DT_NEEDED:', report['needed'], '; relocations:', report['relocation_types'])
    except (OSError, ValueError, subprocess.CalledProcessError) as error:
        parser.exit(1, str(error) + '\n')
