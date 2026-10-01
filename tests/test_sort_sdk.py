"""Execute sealed smoothsort/PRNG and real Mesa C++ TLS sort in MyOS."""
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
from test_elf_dynamic import long_command,assert_live_resources
from make_asset_disk import create
parser=argparse.ArgumentParser();parser.add_argument('--uefi',action='store_true');args=parser.parse_args()
build=runpy.run_path(str(root/'tools/build_format_sdk.py'));out=build['out']
sort=runpy.run_path(str(root/'tools/build_sort_sdk.py'),init_globals={'math_build':build['math']})
archive=root/'build/renderer-evaluation/mesa-25.0.7.tar.xz'
with archive.open('rb') as stream:
    assert hashlib.file_digest(stream,'sha256').hexdigest()=='592272df3cf01e85e7db300c449df5061092574d099da275d19e97ef0510f8a6'
subprocess.run(['tar','-xJf',str(archive),'-C',str(out),
    'mesa-25.0.7/src/util/u_qsort.cpp','mesa-25.0.7/src/util/u_qsort.h',
    'mesa-25.0.7/src/util/detect_os.h'],check=True)
source=out/'mesa-25.0.7'
for check in (True,False):
    subprocess.run(['git','apply',*(['--check'] if check else []),
        '--directory='+str(source.relative_to(root)),str(root/'tools/mesa_sort_target.patch')],cwd=root,check=True)
objects=[]
flags=[flag for flag in build['flags'] if flag not in ('-std=c11','-frounding-math','-ffp-contract=off')]
for name,path in [('mesa',source/'src/util/u_qsort.cpp'),('fixture',root/'user/elf-sort-sdk.cpp')]:
    obj=out/(name+'.o')
    subprocess.run(['g++',*flags,'-std=c++17','-nostdinc++','-fno-exceptions','-fno-rtti',
        '-I'+str(source/'src/util'),'-c',str(path),'-o',str(obj)],check=True);objects.append(obj)
library=out/'sort-fixture.so'
subprocess.run(['ld','-shared','--hash-style=both','-Bsymbolic','-z','now','-z','relro',
    '-z','noexecstack','-e','_start','-o',str(library),*map(str,objects),
    *map(str,sort['objects']),*map(str,build['objects']),*map(str,build['math']['objects'].values())],check=True)
assert '(NEEDED)' not in subprocess.check_output(['readelf','-dW',str(library)],text=True)
disk=out/'assets.img';create(disk,[f'sort={library}'])
boot=root/'build'/('myos-uefi.img' if args.uefi else 'myos-current.img')
image=out/'frozen-boot.img';before=digest(boot);shutil.copyfile(boot,image)
options=dict(ram=64,boot_image=image,asset_disk=disk)
if args.uefi:options.update(ram=128,boot_bus='ide',firmware=(
    Path('/usr/share/OVMF/OVMF_CODE_4M.fd'),Path('/usr/share/OVMF/OVMF_VARS_4M.fd')))
vm=VM('sort-sdk-uefi' if args.uefi else 'sort-sdk',**options)
try:
    vm.wait('myos> ')
    for _ in range(3):
        result=long_command(vm,'elf live sort')
        assert 'ELF exited with status 86' in result,result
        assert 'native sort/random/Mesa TLS nested/concurrency PASS' in result,result
        assert result.index('sort finalize2')<result.index('sort finalize1')
        assert result.count('sort finalize1')==result.count('sort finalize2')==1,result
        assert_live_resources(result)
    print('PASS: native smoothsort guards/large records/context, musl PRNG exact seeds, '
          '8-worker serialized sequence, actual Mesa C++ per-thread/nested callbacks, '
          'reverse atexit once/relaunch/exact resources. NOT crypto/renderer/Cobalt.',out,flush=True)
finally:vm.close()
assert before==digest(boot)==digest(image)
