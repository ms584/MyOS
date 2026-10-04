"""Native pinned Mesa memory queries; no graphics/backend acceptance."""
from pathlib import Path
import argparse
import hashlib
import re
import shutil
import struct
import subprocess
import sys
import tempfile
root=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(root/'tests'));sys.path.insert(0,str(root/'tools'))
from test_system import VM,symbols
from test_elf_dynamic import long_command,assert_live_resources
from make_asset_disk import create
parser=argparse.ArgumentParser();parser.add_argument('--uefi',action='store_true');args=parser.parse_args()
def digest(path):
    with path.open('rb') as stream:return hashlib.file_digest(stream,'sha256').hexdigest()
base=root/'build/mesa-os-info-evaluation';base.mkdir(parents=True,exist_ok=True)
out=Path(tempfile.mkdtemp(prefix='native-',dir=base))
archive=root/'build/renderer-evaluation/mesa-25.0.7.tar.xz'
assert digest(archive)=='592272df3cf01e85e7db300c449df5061092574d099da275d19e97ef0510f8a6'
subprocess.run(['tar','-xJf',str(archive),'-C',str(out)],check=True)
source=out/'mesa-25.0.7'
for check in (True,False):
    subprocess.run(['git','apply',*(['--check'] if check else []),
                    '--directory='+str(source.relative_to(root)),str(root/'tools/mesa_build_target.patch')],
                   cwd=root,check=True)
musl=root/'build/math-evaluation/musl-1.2.6.tar.gz'
assert digest(musl)=='d585fd3b613c66151fc3249e8ed44f77020cb5e6c1e635a616d3f9f82460512a'
header=subprocess.check_output(['tar','-xOf',str(musl),'musl-1.2.6/include/unistd.h'],text=True)
for name,value in (('_SC_PAGE_SIZE',30),('_SC_PAGESIZE',30),('_SC_PHYS_PAGES',85),('_SC_AVPHYS_PAGES',86)):
    assert re.search(r'^#define '+name+r'\s+'+str(value)+r'\b',header,re.M),name
intrinsics=subprocess.check_output(['gcc','-print-file-name=include'],text=True).strip()
flags=['-m64','-Os','-fPIC','-fvisibility=hidden','-ffreestanding','-fno-builtin','-mno-red-zone',
       '-fno-stack-protector','-fno-asynchronous-unwind-tables','-fno-unwind-tables',
       '-ffunction-sections','-fdata-sections','-nostdinc','-isystem',intrinsics,
       '-I'+str(root/'user/sdk'),'-I'+str(source/'include'),'-I'+str(source/'src'),
       '-D__myos__=1','-D_GNU_SOURCE=1','-DHAVE_PTHREAD=1','-DHAVE_STRUCT_TIMESPEC=1',
       '-U__linux__','-U__linux','-Ulinux','-U__gnu_linux__','-Wall','-Wextra','-Werror',
       '-Wno-unused-parameter','-Wno-missing-field-initializers']
objects=[]
for name in ('os_misc','u_dl'):
    obj=out/(name+'.o')
    subprocess.run(['gcc','-std=c11',*flags,'-c',str(source/'src/util'/(name+'.c')),'-o',str(obj)],check=True)
    objects.append(str(obj))
assets=[]
for name,compiler in (('c','gcc'),('cpp','g++')):
    obj=out/(name+'.o');library=out/(name+'.so')
    extra=['-std=c11'] if name=='c' else ['-std=c++17','-nostdinc++','-fno-exceptions','-fno-rtti']
    subprocess.run([compiler,*extra,*flags,'-x','c' if name=='c' else 'c++','-c',
                    str(root/'user/elf-mesa-os-info.c'),'-o',str(obj)],check=True)
    subprocess.run(['ld','-shared','--gc-sections','--hash-style=both','-Bsymbolic',
                    '-z','now','-z','relro','-z','noexecstack','-e','_start','-o',
                    str(library),str(obj),*objects],check=True)
    assert '(NEEDED)' not in subprocess.check_output(['readelf','-dW',str(library)],text=True)
    assets.append(name+'='+str(library))
    if name=='c':
        # This regression intentionally has no hashed exports: undefined
        # imports are not represented by GNU hash's empty buckets.
        original=library.read_bytes();eh=struct.unpack_from('<16sHHIQQQIHHHHHH',original)
        sections=[struct.unpack_from('<IIQQQQIIQQ',original,eh[6]+i*eh[11]) for i in range(eh[12])]
        names=original[sections[eh[13]][4]:sections[eh[13]][4]+sections[eh[13]][5]]
        offsets={names[s[0]:].split(b'\0',1)[0].decode():s[4] for s in sections}
        gh=struct.unpack_from('<IIII',original,offsets['.gnu.hash'])
        buckets=offsets['.gnu.hash']+16+gh[2]*8
        assert all(value==0 for value in struct.unpack_from('<'+'I'*gh[0],original,buckets))
        count=struct.unpack_from('<I',original,offsets['.hash']+4)[0]
        assert count>gh[1]
        for bad_name,offset,value in (('badcount',offsets['.hash']+4,count-1),
                                      ('badgnu',offsets['.gnu.hash']+12,64)):
            damaged=bytearray(original);struct.pack_into('<I',damaged,offset,value)
            bad=out/(bad_name+'.so');bad.write_bytes(damaged)
            assets.append(bad_name+'='+str(bad))
disk=out/'assets.img';create(disk,assets)
boot=root/'build'/('myos-uefi.img' if args.uefi else 'myos-current.img')
before=digest(boot);image=out/'frozen-boot.img';shutil.copyfile(boot,image)
syms=symbols();totals=[]
for ram in ((128,256) if args.uefi else (16,64,256)):
    options=dict(ram=ram,boot_image=image,asset_disk=disk)
    if args.uefi:options.update(boot_bus='ide',firmware=(
        Path('/usr/share/OVMF/OVMF_CODE_4M.fd'),Path('/usr/share/OVMF/OVMF_VARS_4M.fd')))
    vm=VM('mesa-os-info-'+str(ram)+('-uefi' if args.uefi else ''),**options)
    try:
        vm.wait('myos> ')
        total=int.from_bytes(vm.memory(syms['total_count'],8),'little')
        available=vm.memory(syms['available'],8192)
        assert total==sum(byte.bit_count() for byte in available)
        assert 0<total<=ram*1024*1024//4096
        totals.append(total)
        for _ in range(3):
            for name in ('c','cpp'):
                result=long_command(vm,'elf live '+name,timeout=30)
                assert 'ELF exited with status 96' in result,result
                assert 'native Mesa real memory/page queries and unavailable dl PASS' in result,result
                found=re.search(r'native managed pages=(\d+) (\d+)',result)
                assert found and int(found[1])==total and 0<int(found[2])<total,result
                assert 'USER FAULT' not in result,result
                assert_live_resources(result)
        for bad in ('badcount','badgnu'):
            free_before=int.from_bytes(vm.memory(syms['free_count'],8),'little')
            result=long_command(vm,'elf live '+bad,timeout=30)
            assert 'error: invalid executable' in result and 'not executed' in result,result
            assert 'ELF live: started' not in result and 'USER FAULT' not in result,result
            assert free_before==int.from_bytes(vm.memory(syms['free_count'],8),'little')
        print('PASS: native Mesa os-info C/C++,',ram,'MiB, managed pages',total,
              'match PMM bitmap; allocations/free/8workers/relaunch/resources; '
              'empty GNU hash imports and malformed hash rollback',out,flush=True)
    finally:vm.close()
assert totals==sorted(totals) and len(set(totals))==len(totals)
assert before==digest(boot)==digest(image)
