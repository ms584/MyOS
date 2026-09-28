#!/usr/bin/env python3
"""Verify contract coverage against the pinned Cobalt audit (optionally the ELF).

This is a static dependency check, never a Cobalt runtime or YouTube test.
"""
import argparse
from collections import Counter
import csv
import hashlib
import json
from pathlib import Path
import subprocess

ROOT=Path(__file__).resolve().parents[1]

def check(library=None, headers=None):
    audit=json.loads((ROOT/'docs/PHASE33_COBALT_AUDIT.json').read_text())
    with (ROOT/'docs/PHASE331_IMPORTS.csv').open(newline='') as f:
        rows=list(csv.DictReader(f))
    names=[row['symbol'] for row in rows]
    if len(names)!=len(set(names)) or sorted(names)!=audit['undefined_symbols']:
        raise ValueError('contract must cover each pinned import exactly once')
    bindings=Counter(row['binding'] for row in rows)
    if bindings!={'GLOBAL':242,'WEAK':1}:
        raise ValueError(f'unexpected bindings: {bindings}')
    if library:
        with library.open('rb') as f:digest=hashlib.file_digest(f,'sha256').hexdigest()
        if digest!=audit['library_sha256']:
            raise ValueError('library SHA256 differs from pinned release')
        text=subprocess.check_output(['readelf','--dyn-syms','-W',str(library)],text=True)
        imports={}
        for line in text.splitlines():
            fields=line.split()
            if len(fields)>=8 and fields[6]=='UND':imports[fields[7]]=fields[4]
        if imports!={row['symbol']:row['binding'] for row in rows}:
            raise ValueError('ELF imports differ from contract')
    if headers:
        manifest=json.loads((ROOT/'docs/PHASE331_ABI_SOURCES.json').read_text())
        for item in manifest['headers']:
            data=(headers/Path(item['path']).name).read_bytes()
            if hashlib.sha256(data).hexdigest()!=item['sha256']:
                raise ValueError('header mismatch: '+item['path'])
    print(f'PASS: {len(rows)} import contracts; {bindings["GLOBAL"]} global, {bindings["WEAK"]} weak')
    print('Implementation status:',dict(Counter(row['status'] for row in rows)))
    print('Static coverage only; native engine and YouTube acceptance remain open.')

if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--library',type=Path);p.add_argument('--headers',type=Path)
    args=p.parse_args()
    try:check(args.library,args.headers)
    except (OSError,ValueError,subprocess.CalledProcessError) as error:p.exit(1,f'{error}\n')
