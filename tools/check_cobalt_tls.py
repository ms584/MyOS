#!/usr/bin/env python3
"""Static weak TLS call-site audit for the exact pinned x64 Cobalt binary.

This proves only the inspected null-guard pattern, not native TLS/engine execution.
Stream disassembly: never save or log the complete ~99 MiB library disassembly.
"""
import argparse
from collections import deque
import hashlib
import json
from pathlib import Path
import re
import subprocess

ROOT=Path(__file__).resolve().parents[1]
WEAK='_ZTHN2v88internal12trap_handler21g_thread_in_wasm_codeE'
GOT=0x621cc10
PLT=0x60b8570
INSTRUCTION=re.compile(r'^\s*([0-9a-f]+):\s+(\S.*)$')

def audit_instructions(lines):
    previous=deque(maxlen=8);calls=[];references=[]
    for line in lines:
        match=INSTRUCTION.match(line)
        if not match:continue
        address=int(match[1],16);instruction=match[2]
        if re.search(r'#\s+621cc10\b',instruction):references.append(address)
        if re.search(r'\bcall\s+60b8570\s+<'+re.escape(WEAK)+r'@plt>',instruction):
            if len(previous)<2:raise ValueError(f'unguarded weak call at {address:#x}')
            (compare_address,compare),(_,branch)=list(previous)[-2:]
            target=re.match(r'je\s+([0-9a-f]+)\b',branch)
            if not (re.match(r'cmpq\s+\$0x0,',compare) and
                    re.search(r'#\s+621cc10\b',compare) and target and
                    int(target[1],16)>=address+5):
                # One pinned compound block calls twice on the non-NULL path.
                # Its NULL branch skips both calls. Check the entire intervening
                # instruction pattern, not a permissive arbitrary guard interval.
                compound=[
                    (0x22951eb,r'cmpq\s+\$0x0,.*#\s+621cc10\b'),
                    (0x22951f3,r'je\s+2295219\b'),
                    (0x22951f5,r'call\s+60b8570\s+<'+re.escape(WEAK)+r'@plt>'),
                    (0x22951fa,r'lea\s+.*#\s+6244cc0\b'),
                    (0x2295201,r'call\s+60b7da0\s+<__emutls_get_address@plt>'),
                    (0x2295206,r'cmpl\s+\$0x0,\(%rax\)'),
                    (0x2295209,r'je\s+22952e3\b'),
                    (0x229520f,r'mov\s+%rax,%r14')]
                if not (address==0x2295212 and len(previous)==len(compound) and
                        all(actual_address==expected_address and re.match(pattern,text)
                            for (actual_address,text),(expected_address,pattern)
                            in zip(previous,compound)) and (0x22951f5,0x22951eb) in calls):
                    raise ValueError(f'weak call lacks audited null/skip guard at {address:#x}')
                compare_address=0x22951eb
            calls.append((address,compare_address))
        previous.append((address,instruction))
    if not calls:raise ValueError('no guarded weak TLS calls found')
    if sorted(references)!=sorted({compare for _,compare in calls}):
        raise ValueError('unexpected weak GOT reference outside audited call guards')
    return calls

def check(library):
    audit=json.loads((ROOT/'docs/PHASE33_COBALT_AUDIT.json').read_text())
    with library.open('rb') as source:digest=hashlib.file_digest(source,'sha256').hexdigest()
    if digest!=audit['library_sha256']:raise ValueError('not the pinned Cobalt binary')
    relocations=subprocess.check_output(['readelf','-rW',str(library)],text=True)
    rows=[line.split() for line in relocations.splitlines() if WEAK in line]
    if {(int(row[0],16),row[2]) for row in rows}!={
            (GOT,'R_X86_64_GLOB_DAT'),(0x621fc48,'R_X86_64_JUMP_SLOT')}:
        raise ValueError('weak TLS relocation boundary changed')
    symbols=subprocess.check_output(['readelf','-Ws',str(library)],text=True)
    if not any('WEAK' in line and 'UND' in line and line.split()[-1]==WEAK
               for line in symbols.splitlines()):raise ValueError('weak symbol contract changed')
    if not any('__emutls_get_address' in line and 'UND' not in line
               for line in symbols.splitlines()):raise ValueError('in-image emulated TLS missing')
    process=subprocess.Popen(['objdump','-d','--no-show-raw-insn',str(library)],stdout=subprocess.PIPE,text=True)
    try:
        calls=audit_instructions(process.stdout)
        if process.wait()!=0:raise ValueError('objdump failed')
    finally:
        if process.poll() is None:
            process.terminate()
            try:process.wait(timeout=5)
            except subprocess.TimeoutExpired:process.kill();process.wait()
        process.stdout.close()
    print(f'PASS: pinned weak TLS: {len(calls)} direct calls guarded by NULL/skip; two zero-bind relocations')
    print('In-image emulated TLS still requires native pthread/allocator/destructor tests; engine gate OPEN.')
    return len(calls)

if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('library',type=Path)
    args=parser.parse_args()
    try:check(args.library)
    except (OSError,ValueError,subprocess.CalledProcessError) as error:parser.exit(1,f'{error}\n')
