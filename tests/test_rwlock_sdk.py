"""Actual Mesa rwlock, pinned SDK storage and guest realtime time adapter."""
from pathlib import Path
import argparse
import hashlib
import runpy
import shutil
import subprocess
import sys
root=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(root/'tests'));sys.path.insert(0,str(root/'tools'))
from test_system import VM
from test_c11_sdk import digest
from test_elf_dynamic import long_command,assert_live_resources,cancel_live
from make_asset_disk import create
parser=argparse.ArgumentParser();parser.add_argument('--uefi',action='store_true');args=parser.parse_args()
build=runpy.run_path(str(root/'tools/build_format_sdk.py'));out=build['out']
calendar=runpy.run_path(str(root/'tools/build_time_sdk.py'))
archive=root/'build/renderer-evaluation/mesa-25.0.7.tar.xz'
with archive.open('rb') as stream:
    assert hashlib.file_digest(stream,'sha256').hexdigest()=='592272df3cf01e85e7db300c449df5061092574d099da275d19e97ef0510f8a6'
subprocess.run(['tar','-xJf',str(archive),'-C',str(out),
    'mesa-25.0.7/src/util/rwlock.c','mesa-25.0.7/src/util/rwlock.h'],check=True)
source=out/'mesa-25.0.7/src/util';mesa=out/'mesa-rwlock.o'
subprocess.run(['gcc',*build['flags'],'-DHAVE_PTHREAD=1','-c',str(source/'rwlock.c'),'-o',str(mesa)],check=True)
flags=[flag for flag in build['flags'] if flag not in ('-std=c11','-frounding-math','-ffp-contract=off')]
assets=[]
for name,defines in [('rwlock',[]),('rwlock-cancel',['-DRWLOCK_CANCEL=1'])]:
    obj=out/(name+'.o');library=out/(name+'.so')
    subprocess.run(['g++',*flags,'-std=c++17','-nostdinc++','-fno-exceptions','-fno-rtti',
        '-DHAVE_PTHREAD=1','-I'+str(source),*defines,'-c',str(root/'user/elf-rwlock-sdk.cpp'),'-o',str(obj)],check=True)
    subprocess.run(['ld','-shared','--hash-style=both','-Bsymbolic','-z','now','-z','relro',
        '--wrap=clock_gettime','-z','noexecstack','-e','_start','-o',str(library),str(obj),str(mesa),
        *map(str,calendar['objects']),*map(str,build['objects']),*map(str,build['math']['objects'].values())],check=True)
    assert '(NEEDED)' not in subprocess.check_output(['readelf','-dW',str(library)],text=True)
    assets.append(f'{name}={library}')
disk=out/'assets.img';create(disk,assets)
boot=root/'build'/('myos-uefi.img' if args.uefi else 'myos-current.img')
image=out/'frozen-boot.img';before=digest(boot);shutil.copyfile(boot,image)
options=dict(ram=64,boot_image=image,asset_disk=disk)
if args.uefi:options.update(ram=128,boot_bus='ide',firmware=(
    Path('/usr/share/OVMF/OVMF_CODE_4M.fd'),Path('/usr/share/OVMF/OVMF_VARS_4M.fd')))
vm=VM('rwlock-sdk-uefi' if args.uefi else 'rwlock-sdk',**options)
try:
    vm.wait('myos> ')
    for _ in range(3):
        result=long_command(vm,'elf live rwlock')
        assert 'ELF exited with status 87' in result,result
        assert 'native Mesa rwlock/shared readers/exclusive writers/time/C++ ABI PASS' in result,result
        assert_live_resources(result)
        cancel_live(vm,'rwlock-cancel','rwlock blocked reader ready')
    result=long_command(vm,'elf live rwlock')
    assert 'ELF exited with status 87' in result,result;assert_live_resources(result)
    print('PASS: real Mesa/pinned80-byte rwlock guards/defaults/errno, simultaneous '
          'readers/exclusive writer/8-worker shared state, realtime time/output '
          'rollback, C++ aliases, blocked rwlock cancellation/4 relaunches/exact '
          'resources. NOT full pthread attrs/POSIX/renderer/Cobalt.',out,flush=True)
finally:vm.close()
assert before==digest(boot)==digest(image)
