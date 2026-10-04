"""Native bytewise collation and genuine GCC cstring, not complete C++ locales."""
from pathlib import Path
import argparse
import contextlib
import runpy
import shutil
import subprocess
import sys
root=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(root/'tests'));sys.path.insert(0,str(root/'tools'))
from test_c11_sdk import digest
from test_system import VM
from test_elf_dynamic import long_command,assert_live_resources
from make_asset_disk import create
parser=argparse.ArgumentParser();parser.add_argument('--configuration',type=Path,required=True)
parser.add_argument('--uefi',action='store_true');args=parser.parse_args()
cfg=args.configuration.resolve()
assert cfg.is_relative_to((root/'build/cxx-target-evaluation').resolve())
base=root/'build/cstring-evaluation';base.mkdir(parents=True,exist_ok=True)
with (base/'last-build.log').open('w') as log,contextlib.redirect_stdout(log):
    build=runpy.run_path(str(root/'tools/build_cstring_sdk.py'))
out=build['out'];assets=[]
for language in ('c','cpp98','cpp17'):
    obj=out/(language+'.o');library=out/(language+'.so')
    flags=build['flags'] if language=='c' else [f for f in build['flags'] if f not in ('-std=c11','-ffreestanding')]+[
        '-std=c++98' if language=='cpp98' else '-std=c++17','-nostdinc++','-fno-exceptions','-fno-rtti']
    if language!='c':flags=['-I'+str(cfg/'build/include'),'-I'+str(cfg/'build/include/x86_64-unknown-myos'),*flags]
    subprocess.run(['gcc' if language=='c' else 'g++',*flags,'-x','c' if language=='c' else 'c++',
                    '-c',str(root/'user/elf-cstring.c'),'-o',str(obj)],check=True)
    subprocess.run(['ld','-shared','--hash-style=both','-Bsymbolic','-z','now','-z','relro',
                    '-z','noexecstack','-e','_start','-o',str(library),str(obj),str(build['library'])],check=True)
    assert '(NEEDED)' not in subprocess.check_output(['readelf','-dW',str(library)],text=True)
    assets.append(language+'='+str(library))
disk=out/'assets.img';create(disk,assets)
boot=root/'build'/('myos-uefi.img' if args.uefi else 'myos-current.img')
before=digest(boot);image=out/'frozen-boot.img';shutil.copyfile(boot,image)
options=dict(ram=64,boot_image=image,asset_disk=disk)
if args.uefi:options.update(ram=128,boot_bus='ide',firmware=(Path('/usr/share/OVMF/OVMF_CODE_4M.fd'),Path('/usr/share/OVMF/OVMF_VARS_4M.fd')))
vm=VM('cstring-uefi' if args.uefi else 'cstring',**options)
try:
    vm.wait('myos> ')
    for _ in range(2):
        for name in ('c','cpp98','cpp17'):
            result=long_command(vm,'elf live '+name,timeout=45)
            assert 'ELF exited with status 99' in result and 'Cstring bytewise/transform/guards/8workers PASS' in result,result
            assert 'USER FAULT' not in result,result
            assert_live_resources(result)
    print('PASS: native sealed bytewise collation, transform length/order/errno/limits/'
          'page guards/8workers, actual GCC C++98/17 cstring; 6 relaunches/exact resources; '
          'NOT language-sensitive locale/full C++/renderer/Cobalt.',out,flush=True)
finally:vm.close()
assert digest(boot)==before==digest(image)
