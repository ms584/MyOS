"""Native sealed Cstdlib plus real upstream cstdlib headers, not full C++."""
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
base=root/'build/cstdlib-evaluation';base.mkdir(parents=True,exist_ok=True)
with (base/'last-build.log').open('w') as log,contextlib.redirect_stdout(log):
    build=runpy.run_path(str(root/'tools/build_cstdlib_sdk.py'))
out=build['out'];assets=[]
primitives=out/'sdk-string.o'
subprocess.run(['gcc',*build['flags'],'-c',str(root/'user/platform-sdk-string.c'),'-o',str(primitives)],check=True)
for language in ('c','cpp'):
    obj=out/(language+'.o');library=out/(language+'.so')
    flags=build['flags'] if language=='c' else [f for f in build['flags'] if f not in ('-std=c11','-ffreestanding')]+[
        '-std=c++17','-nostdinc++','-fno-exceptions','-fno-rtti']
    # Upstream cstdlib before SDK C headers; no handwritten STL replacements.
    if language=='cpp':flags=['-I'+str(cfg/'build/include'),'-I'+str(cfg/'build/include/x86_64-unknown-myos'),*flags]
    subprocess.run(['gcc' if language=='c' else 'g++',*flags,'-x','c' if language=='c' else 'c++',
                    '-c',str(root/'user/elf-cstdlib.c'),'-o',str(obj)],check=True)
    subprocess.run(['ld','-shared','--hash-style=both','-Bsymbolic','-z','now','-z','relro',
                    '-z','noexecstack','-e','_start','-o',str(library),str(obj),str(primitives),str(build['library'])],check=True)
    assert '(NEEDED)' not in subprocess.check_output(['readelf','-dW',str(library)],text=True)
    assets.append(language+'='+str(library))
disk=out/'assets.img';create(disk,assets)
boot=root/'build'/('myos-uefi.img' if args.uefi else 'myos-current.img')
before=digest(boot);image=out/'frozen-boot.img';shutil.copyfile(boot,image)
options=dict(ram=64,boot_image=image,asset_disk=disk)
if args.uefi:options.update(ram=128,boot_bus='ide',firmware=(Path('/usr/share/OVMF/OVMF_CODE_4M.fd'),Path('/usr/share/OVMF/OVMF_VARS_4M.fd')))
vm=VM('cstdlib-uefi' if args.uefi else 'cstdlib',**options)
try:
    vm.wait('myos> ')
    for _ in range(3):
        for name in ('c','cpp'):
            result=long_command(vm,'elf live '+name,timeout=45)
            assert 'ELF exited with status 98' in result and 'Cstdlib div/UTF8/bounds/8workers PASS' in result,result
            assert 'USER FAULT' not in result,result
            assert_live_resources(result)
    print('PASS: native sealed division signs/extremes, UTF-8 scalar/partial/bulk/invalid/'
          'output bounds/resume/errno/8workers, real upstream C++ cstdlib overloads; '
          'system honestly unavailable,6 relaunches/exact resources; NOT full C++/renderer.',out,flush=True)
finally:vm.close()
assert digest(boot)==before==digest(image)
