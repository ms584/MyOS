"""Run measured renderer string and allocator SDK contracts in MyOS."""
from pathlib import Path
import argparse
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
parser=argparse.ArgumentParser();parser.add_argument('--uefi',action='store_true');args=parser.parse_args()
build=runpy.run_path(str(root/'tools/build_format_sdk.py'));out=build['out']
objects=[]
for language in ('c','c++'):
    obj=out/('string-fixture-'+language+'.o')
    flags=build['flags']+['-D_GNU_SOURCE=1'] if language=='c' else [flag for flag in build['flags'] if flag!='-std=c11']+[
        '-std=c++17','-nostdinc++','-fno-exceptions','-fno-rtti']
    subprocess.run(['gcc' if language=='c' else 'g++',*flags,'-x',language,
        '-c',str(root/'user/elf-string-sdk.c'),'-o',str(obj)],check=True)
    objects.append(obj)
    library=out/('string-fixture-'+language+'.so')
    subprocess.run(['ld','-shared','--hash-style=both','-Bsymbolic','-z','now','-z','relro',
        '-z','noexecstack','-e','_start','-o',str(library),str(obj),
        *map(str,build['objects']),*map(str,build['math']['objects'].values())],check=True)
    assert '(NEEDED)' not in subprocess.check_output(['readelf','-dW',str(library)],text=True)
    # Both variants execute natively: unmangled entrypoint is declared in fixture.
disk=out/'assets.img';create(disk,[f'string-c={out/"string-fixture-c.so"}',f'string-cpp={out/"string-fixture-c++.so"}'])
boot=root/'build'/('myos-uefi.img' if args.uefi else 'myos-current.img');image=out/'frozen-boot.img';before=digest(boot)
shutil.copyfile(boot,image);assert before==digest(boot)==digest(image)
options=dict(ram=64,boot_image=image,asset_disk=disk)
if args.uefi:options.update(ram=128,boot_bus='ide',firmware=(
    Path('/usr/share/OVMF/OVMF_CODE_4M.fd'),Path('/usr/share/OVMF/OVMF_VARS_4M.fd')))
vm=VM('string-sdk-uefi' if args.uefi else 'string-sdk',**options)
try:
    vm.wait('myos> ')
    for name in ('string-c','string-cpp'):
        for _ in range(3):
            result=long_command(vm,'elf live '+name)
            assert 'ELF exited with status 82' in result,result
            assert 'native SDK strings/37281 overlaps/unsigned compare/allocator ABI PASS' in result,result
            assert 'native string extensions/C-locale/8-worker tokens/math constants PASS' in result,result
            assert_live_resources(result)
    print('PASS: native C11/C++17 SDK string/allocator ABI, 37281 overlapping moves, '
          '65536 unsigned comparisons, padding/truncation/search/empty cases, '
          'alignment/error/overflow/zero-fill/free, string extensions/C-locale/'
          '8-worker tokens/math constants/relaunch/resources. NOT Cobalt.',out,flush=True)
finally:vm.close()
assert before==digest(boot)==digest(image)
