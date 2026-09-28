#!/usr/bin/env python3
"""Create an exclusive, read-only MyOS asset volume; never overwrite a disk.

Usage: make_asset_disk.py OUTPUT NAME=FILE [NAME=FILE ...]
No host filesystem paths are embedded in the volume.
"""
import argparse
from pathlib import Path
import re
import shutil
import struct

def create(output, specs):
    if not 1 <= len(specs) <= 32:
        raise ValueError('requires 1..32 assets')
    entries=[]; names=set(); offset=4096
    for spec in specs:
        name,sep,path=spec.partition('=')
        if not sep or not re.fullmatch(r'[A-Za-z0-9_.-]{1,47}',name) or name in names:
            raise ValueError('invalid/duplicate NAME=FILE')
        source=Path(path); size=source.stat().st_size
        if not source.is_file() or not size:
            raise ValueError('asset must be a nonempty regular file')
        names.add(name); entries.append((name,source,offset,size))
        offset=(offset+size+511)&~511
    total=max(1024*1024,offset)
    if total//512>0x0fffffff:
        raise ValueError('asset volume exceeds LBA28 capacity')
    with Path(output).open('xb') as out:
        out.truncate(total)
        out.write(struct.pack('<16sIIQ',b'MYOS-ASSET-v1',1,len(entries),total))
        out.seek(512)
        for name,source,position,size in entries:
            out.write(struct.pack('<48sQQ',name.encode(),position,size))
        for name,source,position,size in entries:
            out.seek(position)
            with source.open('rb') as inp:
                remaining=size
                while remaining:
                    data=inp.read(min(remaining,1024*1024))
                    if not data:raise ValueError('source changed during packing')
                    out.write(data);remaining-=len(data)
                if inp.read(1):raise ValueError('source changed during packing')
    return total

if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('output',type=Path);parser.add_argument('assets',nargs='+')
    args=parser.parse_args()
    try:print(f'Created {args.output}: {create(args.output,args.assets)} bytes (read-only assets)')
    except (OSError,ValueError) as error:parser.exit(1,f'{error}\n')
