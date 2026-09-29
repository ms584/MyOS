"""QEMU single-object dynamic ELF acceptance, including fail-closed startup."""
import argparse
import csv
import hashlib
import json
from pathlib import Path
import struct
import subprocess
import sys
import time
from test_system import VM,OUT,ROOT
from test_shell import command,type_text
sys.path.insert(0,str(ROOT/'tools'))
from make_asset_disk import create

def implemented_exports():
    with (ROOT/'docs/PHASE331_IMPORTS.csv').open(newline='') as source:
        return {row['symbol'] for row in csv.DictReader(source)
                if row['status']=='implemented-tested'}

def digest(path):
    with path.open('rb') as f:return hashlib.file_digest(f,'sha256').hexdigest()

def long_command(vm,text,timeout=240):
    offset=len(vm.log.read_text());type_text(vm,text+'\n');end=time.monotonic()+timeout
    while time.monotonic()<end:
        result=vm.log.read_text()[offset:]
        assert 'PANIC' not in result,result
        if result.endswith('myos> '):return result
        time.sleep(.1)
    raise AssertionError((text,vm.log.read_text()[-4000:]))

def fixtures():
    out=OUT/'elf-dynamic';out.mkdir(exist_ok=True)
    objects=[]
    for source in ('elf-dynamic','elf-dynamic-export'):
        obj=out/(source+'.o');objects.append(str(obj))
        subprocess.run(['nasm','-f','elf64',f'user/{source}.asm','-o',str(obj)],check=True)
    library=out/'fixture.so'
    subprocess.run(['ld','-shared','--hash-style=both','-z','now','-z','relro','-e','_start',
                    '-init','init_first','-o',str(library),*objects],check=True)
    original=library.read_bytes();phoff=struct.unpack_from('<Q',original,32)[0]
    phnum=struct.unpack_from('<H',original,56)[0]
    ph=[struct.unpack_from('<IIQQQQQQ',original,phoff+i*56) for i in range(phnum)]
    def fileoff(va):
        for p in ph:
            if p[0]==1 and p[3]<=va<p[3]+p[5]:return p[2]+va-p[3]
        raise ValueError(hex(va))
    dynamic=next(p for p in ph if p[0]==2);tags={};positions={}
    for off in range(dynamic[2],dynamic[2]+dynamic[5],16):
        tag,value=struct.unpack_from('<qQ',original,off)
        if not tag:break
        tags[tag]=value;positions[tag]=off
    rel=[]
    for start,size in ((tags[7],tags[8]),(tags[23],tags[2])):
        for off in range(fileoff(start),fileoff(start)+size,24):rel.append((off,*struct.unpack_from('<QQq',original,off)))
    assert {info&0xffffffff for _,_,info,_ in rel}=={1,6,7,8},rel
    variants={}
    def mutate(name,offset,fmt,value):
        data=bytearray(original);struct.pack_into(fmt,data,offset,value);variants[name]=data
    data=bytearray(original);struct.pack_into('<Q',data,40,0);struct.pack_into('<HHH',data,58,0,0,0);variants['sectionless']=data
    # Exercise both dynamic hash formats without a section table dependency.
    for name,tag in [('gnu-only',4),('sysv-only',0x6ffffef5)]:
        data=bytearray(original);struct.pack_into('<qQ',data,positions[tag],21,0);variants[name]=data
    symoff=fileoff(tags[6]);count=struct.unpack_from('<II',original,fileoff(tags[4]))[1]
    weak=next(symoff+i*24 for i in range(count) if original[symoff+i*24+4]>>4==2)
    mutate('missing',weak+4,'B',(original[weak+4]&15)|16)
    mutate('bad-rela-size',positions[9]+8,'Q',16)
    mutate('duplicate-tag',positions[4],'q',5)
    mutate('needed',positions[4],'q',1)
    mutate('unknown-reloc',rel[0][0]+8,'Q',99)
    mutate('bad-target',rel[0][0],'Q',2**64-1)
    text=next(p for p in ph if p[0]==1 and p[1]&1)
    mutate('text-target',rel[0][0],'Q',text[3])
    mutate('metadata-target',rel[0][0],'Q',tags[7])
    absolute=next(item for item in rel if item[2]&0xffffffff==1)
    mutate('bad-symbol',absolute[0]+8,'Q',((count+1)<<32)|1)
    data=bytearray(original)
    value=struct.unpack_from('<Q',original,symoff+(absolute[2]>>32)*24+8)[0]
    struct.pack_into('<Qq',data,absolute[0]+8,1,0x8000000000+value)
    variants['zero-symbol']=data
    data=bytearray(original);struct.pack_into('<qQ',data,positions[4],13,tags[12]);variants['finalizer']=data
    ctor=next(item for item in rel if item[1]==tags[25])
    data_segment=next(p for p in ph if p[0]==1 and p[1]&2)
    mutate('bad-constructor',ctor[0]+16,'q',data_segment[3])
    relro=next(p for p in ph if p[0]==0x6474e552)
    entry=struct.unpack_from('<Q',original,24)[0]
    data=bytearray(original);code=b'\x48\xb8'+struct.pack('<Q',0x8000000000+relro[3])+b'\xc6\x00\x00'
    data[fileoff(entry):fileoff(entry)+len(code)]=code;variants['relro-fault']=data
    for name,code in [('constructor-fault',b'\x0f\x0b'),('constructor-loop',b'\xeb\xfe')]:
        data=bytearray(original);off=fileoff(tags[12]);data[off:off+len(code)]=code;variants[name]=data
    platform_specs=[]
    for source,name in (('elf-platform-allocator','allocator'),('elf-platform-vm','vm')):
        obj=out/(source+'.o');image=out/(name+'.so')
        subprocess.run(['nasm','-f','elf64',f'user/{source}.asm','-o',str(obj)],check=True)
        subprocess.run(['ld','-shared','--hash-style=both','-z','now','-z','relro','-e','_start',
                        '-o',str(image),str(obj)],check=True)
        platform_specs.append(f'{name}={image}')
    pthread_obj=out/'elf-platform-pthread.o';pthread_image=out/'pthread.so'
    subprocess.run(['gcc','-m64','-std=c11','-Os','-fPIC','-ffreestanding','-fno-builtin',
                    '-fno-stack-protector','-fno-asynchronous-unwind-tables','-fno-unwind-tables',
                    '-Wall','-Wextra','-Werror','-c','user/elf-platform-pthread.c',
                    '-o',str(pthread_obj)],check=True)
    subprocess.run(['ld','-shared','--hash-style=both','-z','now','-z','relro','-e','_start',
                    '-o',str(pthread_image),str(pthread_obj)],check=True)
    platform_specs.append(f'pthread={pthread_image}')
    specs=[f'dynamic={library}',*platform_specs]
    for name,data in variants.items():
        path=out/(name+'.so');path.write_bytes(data);specs.append(f'{name}={path}')
    disk=out/'assets.img';disk.unlink(missing_ok=True);create(disk,specs)
    return out,disk

def exercise(cobalt=None):
    out,disk=fixtures();before=digest(disk);vm=VM('elf-dynamic',disk=disk,ram=16)
    try:
        vm.wait('myos> ')
        output=command(vm,'elf link dynamic','staged image released; not executed')
        assert 'relative=2 absolute=1 global=2 jump=1 skipped=0' in output,output
        assert 'constructor executed' not in output,output
        for name in ('dynamic','sectionless','gnu-only','sysv-only','zero-symbol'):
            output=command(vm,'elf run '+name,'ELF exited with status 32')
            assert 'constructor executed in Ring 3' in output,output
            assert 'ELF dynamic: four relocations' in output,output
        for name in ('bad-rela-size','duplicate-tag','bad-target','text-target','metadata-target','bad-symbol','bad-constructor'):
            output=command(vm,'elf run '+name,'error: invalid executable')
            assert 'constructor executed' not in output,output
        for name in ('missing','needed','unknown-reloc','finalizer'):
            output=command(vm,'elf run '+name,'unsupported runtime feature; not executed')
            assert 'constructor executed' not in output,output
        output=command(vm,'elf run allocator','ELF exited with status 33')
        assert 'ELF platform allocator semantics PASS' in output,output
        assert all(f'ELF platform: {name}' in output for name in
                   ('aligned_alloc','calloc','free','malloc','malloc_usable_size','posix_memalign','realloc')),output
        output=command(vm,'elf run vm','ELF exited with status 34')
        assert 'ELF POSIX VM and per-thread errno PASS' in output,output
        assert all(f'ELF platform: {name}' in output for name in
                   ('__errno_location','madvise','mincore','mmap','mprotect','msync','munmap')),output
        output=command(vm,'elf run pthread','ELF exited with status 35')
        assert 'ELF pthread lifecycle, sync and TLS PASS' in output,output
        output=command(vm,'elf run relro-fault','error: invalid executable')
        assert 'vector=14 error=0x0000000000000007' in output and 'ELF RELRO: protected' in output,output
        output=command(vm,'elf run constructor-fault','error: invalid executable')
        assert 'vector=6 error=0x0000000000000000' in output and 'ELF dynamic: four relocations' not in output,output
        command(vm,'elf run constructor-loop','error: app timed out')
        for _ in range(3):command(vm,'elf run dynamic','ELF exited with status 32')
        command(vm,'run /bin/hello','App exited with status 42')
        print('PASS: four dynamic relocation types; SysV/GNU hash; sectionless startup; Ring 3 constructors; RELRO fault; invalid/missing-symbol rollback',flush=True)
    finally:vm.close()
    assert digest(disk)==before
    if cobalt:
        audit=json.loads((ROOT/'docs/PHASE33_COBALT_AUDIT.json').read_text())
        assert digest(cobalt)==audit['library_sha256'],'wrong Cobalt build'
        disk=out/'cobalt.img';disk.unlink(missing_ok=True);create(disk,[f'cobalt={cobalt}']);before=digest(disk)
        vm=VM('elf-dynamic-cobalt',disk=disk,ram=256)
        try:
            vm.wait('myos> ')
            output=long_command(vm,'elf link cobalt')
            assert 'relative=117653 absolute=20 global=17 jump=626' in output,output[-4000:]
            exports=implemented_exports()
            expected=[name for name in audit['undefined_symbols']
                      if not name.startswith('_ZTH') and name not in exports]
            assert f'missing={len(expected)} weak-zero=1' in output,output[-4000:]
            assert 'ELF RELRO: protected' in output,output[-4000:]
            assert 'staged image released; not executed' in output,output[-4000:]
            imports=[line.split(': ',1)[1] for line in output.splitlines() if line.startswith('ELF missing: ')]
            assert sorted(imports)==sorted(expected)
            command(vm,'run /bin/hello','App exited with status 42')
            print(f'PASS: actual Cobalt mapped and relocations processed in MyOS; {len(exports)} tested runtime imports resolved, {len(expected)} missing imports block execution; all staged pages reclaimed',flush=True)
        finally:vm.close()
        assert digest(disk)==before
        vm=VM('elf-dynamic-cobalt-oom',disk=disk,ram=64)
        try:
            vm.wait('myos> ')
            output=long_command(vm,'elf link cobalt')
            assert 'error: out of memory' in output,output
            command(vm,'run /bin/hello','App exited with status 42')
            print('PASS: actual large image OOM at 64 MiB rolls back and leaves shell/apps usable',flush=True)
        finally:vm.close()

if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('--cobalt',type=Path);exercise(p.parse_args().cobalt)
