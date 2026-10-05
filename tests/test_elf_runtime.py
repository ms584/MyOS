"""Native read-only assets and Ring 3 ELF execution; optional real Cobalt audit."""
import argparse
import hashlib
import json
from pathlib import Path
import struct
import subprocess
import sys
from test_system import VM, OUT, ROOT
from test_shell import command
sys.path.insert(0,str(ROOT/'tools'))
from make_asset_disk import create
from create_data_disk import create_image

def sha(path):
    with path.open('rb') as f:return hashlib.file_digest(f,'sha256').hexdigest()

def fixtures():
    out=OUT/'elf-assets';out.mkdir(exist_ok=True)
    subprocess.run(['nasm','-f','elf64','user/elf-fixture.asm','-o',str(out/'fixture.o')],check=True)
    subprocess.run(['ld','-m','elf_x86_64','--build-id=none','-T','user/elf-fixture.ld','-o',str(out/'fixture.elf'),str(out/'fixture.o')],check=True)
    original=(out/'fixture.elf').read_bytes();assert len(original)>65536
    ph=struct.unpack_from('<Q',original,32)[0]
    variants={}
    def change(name,offset,fmt,value):
        data=bytearray(original);struct.pack_into(fmt,data,offset,value);variants[name]=data
    change('bad-magic',0,'B',0)
    change('bad-machine',18,'H',3)
    change('bad-phoff',32,'Q',2**64-32)
    change('bad-filesz',ph+32,'Q',2**64-1)
    change('bad-address',ph+16,'Q',0x10000)
    change('bad-entry',24,'Q',0x8000011000)
    change('bad-wx',ph+4,'I',7)
    change('bad-align',ph+48,'Q',3)
    change('bad-overlap',ph+56+16,'Q',0x8000000000)
    change('unsupported-interp',ph,'I',3)
    change('unsupported-dynamic',16,'H',3) # wrong bias means invalid layout, never executed
    data=bytearray(original)
    # Inflate last segment BSS to exercise partial-allocation cleanup at 16 MiB.
    struct.pack_into('<Q',data,ph+2*56+40,32*1024*1024)
    variants['oom']=data
    text_off=struct.unpack_from('<Q',original,ph+8)[0]
    for name,code in {
        'rx-fault': bytes.fromhex('48b80000000080000000c60000'),
        'nx-fault': bytes.fromhex('48b80010001080000000ffe0'),
        'loop': bytes.fromhex('ebfe'),
    }.items():
        data=bytearray(original);data[text_off:text_off+len(code)]=code;variants[name]=data
    variants['truncated']=original[:80]
    specs=[f'fixture={out/"fixture.elf"}']
    for name,data in variants.items():
        path=out/(name+'.elf');path.write_bytes(data);specs.append(f'{name}={path}')
    disk=out/'assets.img';disk.unlink(missing_ok=True);create(disk,specs)
    return out,disk

def exercise(cobalt=None):
    out,disk=fixtures();before=sha(disk)
    vm=VM('elf-runtime',disk=disk,ram=16)
    try:
        vm.wait('myos> ')
        command(vm,'assets','fixture ')
        command(vm,'elf inspect fixture','ELF64: ET_EXEC')
        for _ in range(3):
            result=command(vm,'elf run fixture','ELF exited with status 33')
            assert 'ELF Ring 3: data, BSS and >64 KiB asset PASS' in result,result
        for name in ('bad-magic','bad-machine','bad-phoff','bad-filesz','bad-address','bad-entry','bad-wx','bad-align','bad-overlap','unsupported-dynamic','truncated'):
            command(vm,'elf run '+name,'error: invalid executable')
        command(vm,'elf run unsupported-interp','unsupported runtime feature; not executed')
        command(vm,'elf run oom','error: out of memory')
        for name in ('rx-fault','nx-fault'):
            result=command(vm,'elf run '+name,'error: invalid executable')
            assert 'USER FAULT' in result,result
        command(vm,'elf run loop','error: app timed out')
        command(vm,'elf run fixture','ELF exited with status 33')
        command(vm,'run /bin/hello','App exited with status 42')
        command(vm,'write /home/check RAM-only')
        command(vm,'sync')
        print('PASS: native ELF >64 KiB, BSS/data, RX/NX faults, malformed inputs, OOM/timeout cleanup, legacy apps',flush=True)
    finally:vm.close()
    assert sha(disk)==before,'asset disk modified'
    # A mutable snapshot disk and read-only asset volume coexist on primary IDE.
    mutable=out/'data.img';mutable.unlink(missing_ok=True);create_image(mutable)
    vm=VM('elf-two-disks',disk=mutable,asset_disk=disk)
    try:
        vm.wait('myos> ')
        command(vm,'write /home/coexist saved')
        command(vm,'elf run fixture','ELF exited with status 33')
        command(vm,'cat /home/coexist','saved')
    finally:vm.close()
    vm=VM('elf-two-disks-reboot',disk=mutable,asset_disk=disk)
    try:
        vm.wait('myos> ')
        command(vm,'cat /home/coexist','saved')
        command(vm,'elf run fixture','ELF exited with status 33')
    finally:vm.close()
    assert sha(disk)==before
    print('PASS: read-only assets coexist with persistent data disk; both survive reboot',flush=True)
    # Corrupt table lengths must not become unchecked disk accesses.
    legacy=out/'legacy-volume.img';legacy.unlink(missing_ok=True)
    create(legacy,[f'fixture={out/"fixture.elf"}'],runtime=None,version=1)
    for source in (legacy,disk):
        data=bytearray(source.read_bytes());version=struct.unpack_from('<I',data,16)[0]
        assert version in (1,2)
        malformed=out/f'bad-volume-v{version}.img'
        struct.pack_into('<Q',data,512+(48 if version==1 else 240),2**64-512)
        malformed.write_bytes(data);malformed_before=sha(malformed)
        vm=VM(f'elf-bad-volume-v{version}',disk=malformed)
        try:
            vm.wait('myos> ')
            command(vm,'assets','unavailable or invalid volume')
            command(vm,'elf run fixture','error: invalid executable')
        finally:vm.close()
        assert sha(malformed)==malformed_before,'malformed volume modified'
    if cobalt:
        audit=json.loads((ROOT/'docs/PHASE33_COBALT_AUDIT.json').read_text())
        assert sha(cobalt)==audit['library_sha256'],'not the pinned Cobalt binary'
        large=out/'cobalt.img';large.unlink(missing_ok=True);create(large,[f'cobalt={cobalt}'])
        before=sha(large);vm=VM('elf-cobalt-audit',disk=large,ram=64)
        try:
            vm.wait('myos> ')
            command(vm,'assets','103391688 bytes')
            result=command(vm,'elf inspect cobalt','ELF imports: strong=242 weak=1')
            names=[]
            for line in result.splitlines():
                if line.startswith(('ELF unresolved: ','ELF weak: ')):names.append(line.split(': ',1)[1])
            assert sorted(names)==audit['undefined_symbols'],names
            command(vm,'elf run cobalt','unsupported runtime feature; not executed')
            command(vm,'run /bin/hello','App exited with status 42')
        finally:vm.close()
        assert sha(large)==before
        print('PASS: MyOS read actual 103 MB ELF metadata and all 243 imports; unresolved engine execution blocked',flush=True)

if __name__=='__main__':
    parser=argparse.ArgumentParser();parser.add_argument('--cobalt',type=Path)
    exercise(parser.parse_args().cobalt)
