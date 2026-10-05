#!/usr/bin/env python3
"""Exclusive readonly v1/v2 asset packaging; v2 has a private runtime extent.

v1 remains flat/32 files. v2 supports1024 sorted hierarchical files, paths<=239
ASCII bytes; long resource enumeration uses the dedicated target asset ABI.
The optional runtime extent is never an enumerated application file.
"""
import argparse
import hashlib
import json
from pathlib import Path
import re
import struct

def create(output,specs,*,runtime='auto',version=None):
    root=Path(__file__).resolve().parents[1]
    if runtime=='auto':
        manifest=root/'build/platform-runtime.manifest.json'
        runtime=None
        if manifest.exists():
            data=json.loads(manifest.read_text())
            if data['external']:
                runtime=root/'build/platform-runtime.mrt'
                encoded=runtime.read_bytes()
                if len(encoded)!=data['packed_bytes'] or hashlib.sha256(encoded).hexdigest()!=data['packed_sha256']:
                    raise ValueError('runtime/manifest mismatch; rebuild before packaging')
    if version is None:version=2 if runtime is not None else 1
    if version not in (1,2) or (version==1 and runtime is not None):
        raise ValueError('runtime extent requires v2')
    limit=32 if version==1 else 1024
    if not 1<=len(specs)<=limit:raise ValueError(f'requires1..{limit} assets')
    files=[];names=set()
    for spec in specs:
        name,sep,path=spec.partition('=')
        valid=bool(re.fullmatch(r'[A-Za-z0-9_.-]{1,47}',name)) if version==1 else (
            1<=len(name)<=239 and all(re.fullmatch(r'[A-Za-z0-9_.-]{1,239}',part)
                                     and part not in ('.','..') for part in name.split('/')))
        if not sep or not valid or name in names:raise ValueError('invalid/duplicate NAME=FILE')
        source=Path(path);size=source.stat().st_size
        if not source.is_file() or not size:raise ValueError('asset must be a nonempty regular file')
        names.add(name);files.append((name,source,size))
    if version==2:
        files.sort(key=lambda item:item[0])
        for name in names:
            parts=name.split('/')
            if any('/'.join(parts[:i]) in names for i in range(1,len(parts))):
                raise ValueError('file/directory path collision')
    width=64 if version==1 else 256
    offset=max(4096,(512+len(files)*width+511)&~511)
    entries=[]
    for name,source,size in files:
        entries.append((name,source,offset,size));offset=(offset+size+511)&~511
    runtime_offset=runtime_size=0
    if runtime is not None:
        runtime=Path(runtime);runtime_size=runtime.stat().st_size
        if not runtime.is_file() or not 16<=runtime_size<=65536+512+16:
            raise ValueError('invalid runtime extent')
        runtime_offset=offset;offset=(offset+runtime_size+511)&~511
    total=max(1024*1024,offset)
    if total//512>0x0fffffff:raise ValueError('asset volume exceeds LBA28')
    with Path(output).open('xb') as out:
        out.truncate(total)
        out.write(struct.pack('<16sIIQ',f'MYOS-ASSET-v{version}'.encode(),version,len(entries),total))
        if version==2:out.write(struct.pack('<QQ',runtime_offset,runtime_size))
        out.seek(512)
        for name,source,position,size in entries:
            out.write(struct.pack('<48sQQ' if version==1 else '<240sQQ',name.encode(),position,size))
        streams=[(source,position,size) for name,source,position,size in entries]
        if runtime is not None:streams.append((runtime,runtime_offset,runtime_size))
        for source,position,size in streams:
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
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('output',type=Path);p.add_argument('assets',nargs='+')
    p.add_argument('--runtime',default='auto');p.add_argument('--version',type=int,choices=(1,2))
    args=p.parse_args()
    runtime=None if args.runtime=='none' else args.runtime
    try:print(f'Created {args.output}: {create(args.output,args.assets,runtime=runtime,version=args.version)} bytes')
    except (OSError,ValueError) as error:p.exit(1,f'{error}\n')
