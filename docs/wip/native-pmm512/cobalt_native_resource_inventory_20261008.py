"""Diagnostic kernel metadata only, at idle shell; no application payloads."""
import json
import struct
import subprocess
from pathlib import Path

def capture(vm, out, label):
    entries = {}
    for line in subprocess.check_output(['nm', '-S', 'build/kernel.elf'], text=True).splitlines():
        fields = line.split()
        if len(fields) == 4:
            base = fields[3].split('.lto_priv.')[0]
            if base in ('nodes', 'heap'):
                assert base not in entries, 'ambiguous diagnostic symbol'
                entries[base] = (int(fields[0], 16), int(fields[1], 16))
    # fs/ramfs.c struct node: x86-64 layout, FS_NAME32/FS_NODES64.
    address, size = entries['nodes']
    assert size == 80 * 64, 'RAMFS diagnostic layout changed'
    raw = vm.memory(address, size)
    nodes = []
    for index in range(64):
        item = raw[index*80:(index+1)*80]
        if not item[0]:
            continue
        parent, kind, references = struct.unpack_from('<III', item, 4)
        length, capacity = struct.unpack_from('<QQ', item, 16)
        data, shared = struct.unpack_from('<QQ', item, 64)
        nodes.append(dict(index=index, parent=parent, kind=kind,
            references=references, unlinked=bool(item[1]), size=length,
            capacity=capacity, name=item[32:64].split(b'\0')[0].decode(),
            data=data, shared=shared))
    address, size = entries['heap']
    assert size == 8
    cursor = int.from_bytes(vm.memory(address, 8), 'little')
    blocks = []
    seen = set()
    while cursor:
        assert cursor not in seen and len(seen)<4096, 'invalid heap chain'
        seen.add(cursor)
        length, following, previous, free = struct.unpack('<QQQQ', vm.memory(cursor, 32))
        if not free:
            blocks.append(dict(address=cursor+32, size=length,
                ramfs_nodes=[node['index'] for node in nodes if cursor+32 in (node['data'], node['shared'])]))
        cursor = following
    result = dict(diagnostic_only=True,label=label,nodes=nodes,blocks=blocks,
        heap_allocated=sum(block['size'] for block in blocks),
        ramfs_allocated=sum(block['size'] for block in blocks if block['ramfs_nodes']))
    (out / ('resources-'+label+'.json')).write_text(json.dumps(result,indent=2))
    print('RESOURCE_METADATA', label, result['heap_allocated'], 'RAMFS',result['ramfs_allocated'],flush=True)
