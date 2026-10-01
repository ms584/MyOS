"""Isolated native guard provider test, with an immutable copy of boot image.

Never rebuilds or edits the regression's shared boot image.
"""
import hashlib
import argparse
import shutil
import subprocess
import sys
import tempfile
from pathlib import Path

root=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(root/'tests'))
sys.path.insert(0,str(root/'tools'))
from test_system import VM
from test_elf_dynamic import long_command,assert_live_resources,cancel_live
from make_asset_disk import create

parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--uefi',action='store_true')
args=parser.parse_args()
guard_source=root/'user/platform-cxx-guard.c'
fixture_source=root/'user/elf-cxx-guard.cpp'
out=root/'build/cxx-guard-evaluation'
out.mkdir(parents=True,exist_ok=True)
out=Path(tempfile.mkdtemp(prefix='native-',dir=out))
def digest(path):
    with path.open('rb') as stream:return hashlib.file_digest(stream,'sha256').hexdigest()
boot=root/'build'/('myos-uefi.img' if args.uefi else 'myos-current.img')
image=out/'frozen-boot.img'
before=digest(boot)
shutil.copyfile(boot,image)
assert before==digest(boot)==digest(image),'Shared image changed during snapshot; do not test partial bytes'
print('Immutable native boot image:',before,str(image),flush=True)
flags=['-m64','-Os','-fPIC','-ffreestanding','-fno-builtin','-mno-red-zone',
       '-fno-stack-protector','-fno-asynchronous-unwind-tables','-fno-unwind-tables',
       '-Wall','-Wextra','-Werror']
subprocess.run(['gcc','-std=c11',*flags,'-c',str(guard_source),'-o',str(out/'guard-native.o')],check=True)
specs=[]
for mode in range(3):
    obj=out/f'fixture{mode}.o';library=out/f'guard{mode}.so'
    subprocess.run(['g++','-std=c++17',*flags,'-fno-exceptions','-fno-rtti','-fno-use-cxa-atexit',
                    '-Wno-unused-function',f'-DGUARD_MODE={mode}',
                    '-c',str(fixture_source),'-o',str(obj)],check=True)
    subprocess.run(['ld','-shared','--hash-style=both','-z','now','-z','relro','-e','_start',
                    '-o',str(library),str(out/'guard-native.o'),str(obj)],check=True)
    dynamic=subprocess.check_output(['readelf','-dW',str(library)],text=True)
    assert 'NEEDED' not in dynamic,'Unexpected host SDK dependency'
    symbols=subprocess.check_output(['readelf','--dyn-syms','-W',str(library)],text=True)
    assert '__cxa_guard_acquire' in symbols and '__cxa_guard_release' in symbols
    specs.append(f'guard{mode}={library}')
disk=out/'guard-assets.img'
create(disk,specs)
options=dict(ram=64,boot_image=image,asset_disk=disk)
if args.uefi:
    options.update(ram=128,boot_bus='ide',
                   firmware=(Path('/usr/share/OVMF/OVMF_CODE_4M.fd'),
                             Path('/usr/share/OVMF/OVMF_VARS_4M.fd')))
vm=VM('cxx-guard-isolated-uefi' if args.uefi else 'cxx-guard-isolated',**options)
try:
    vm.wait('myos> ')
    for _ in range(3):
        result=long_command(vm,'elf live guard0')
        assert 'ELF exited with status 61' in result and 'native compiler C++ guard' in result,result
        assert_live_resources(result)
    result=long_command(vm,'elf live guard1')
    assert 'USER FAULT' in result and 'vector=6 ' in result,result
    assert_live_resources(result)
    cancel_live(vm,'guard2','C++ guard blocked cancel ready',timeout=15)
    result=long_command(vm,'elf live guard0')
    assert 'ELF exited with status 61' in result,result
    assert_live_resources(result)
    print('PASS: NATIVE compiler C++ guard contention/publication, abort wakes/retries, recursion fault, blocked-worker cancellation/relaunch and exact resources. NOT renderer/Cobalt acceptance.',flush=True)
finally:vm.close()
assert digest(image)==before,'Immutable test image changed'
