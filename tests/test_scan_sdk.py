"""Execute sealed conversions/memory scanf in MyOS, not Cobalt FILE ABI."""
from pathlib import Path
import argparse
import contextlib
import hashlib
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
from scan_oracle import generate
parser=argparse.ArgumentParser();parser.add_argument('--uefi',action='store_true');args=parser.parse_args()
base=root/'build/scan-evaluation';base.mkdir(parents=True,exist_ok=True)
with (base/'last-build.log').open('w') as log,contextlib.redirect_stdout(log):
    build=runpy.run_path(str(root/'tools/build_format_sdk.py'));out=build['out']
    scan=runpy.run_path(str(root/'tools/build_scan_sdk.py'),init_globals={'math_build':build['math']})
count=generate(out/'scan-vectors.h');assets=[]
archive=root/'build/renderer-evaluation/mesa-25.0.7.tar.xz'
with archive.open('rb') as stream:
    assert hashlib.file_digest(stream,'sha256').hexdigest()=='592272df3cf01e85e7db300c449df5061092574d099da275d19e97ef0510f8a6'
subprocess.run(['tar','-xJf',str(archive),'-C',str(out),
    'mesa-25.0.7/src/util','mesa-25.0.7/src/c11','mesa-25.0.7/include'],check=True)
mesa=out/'mesa-strtod.o';mesa_source=out/'mesa-25.0.7/src'
subprocess.run(['gcc',*build['flags'],'-DHAVE_PTHREAD=1','-DHAVE_STRTOF=1','-DHAVE_STRUCT_TIMESPEC=1',
    '-DUSE_GCC_ATOMIC_BUILTINS=1','-I'+str(mesa_source),'-I'+str(mesa_source.parent/'include'),
    '-c',str(mesa_source/'util/strtod.c'),'-o',str(mesa)],check=True)
for language in ('c','c++'):
    obj=out/('scan-fixture-'+language+'.o');library=out/('scan-fixture-'+language+'.so')
    flags=build['flags'] if language=='c' else [flag for flag in build['flags'] if flag not in (
        '-std=c11','-frounding-math','-ffp-contract=off')]+[
        '-std=c++17','-nostdinc++','-fno-exceptions','-fno-rtti']
    subprocess.run(['gcc' if language=='c' else 'g++',*flags,
        '-Wno-error=format','-Wno-error=format-zero-length','-x',language,
        '-c',str(root/'user/elf-scan-sdk.c'),'-o',str(obj)],check=True)
    subprocess.run(['ld','-shared','--hash-style=both','-Bsymbolic','-z','now','-z','relro',
        '--wrap=malloc','--wrap=realloc','-z','noexecstack','-e','_start','-o',str(library),
        str(obj),str(mesa),*map(str,build['objects']),*map(str,build['math']['objects'].values()),str(scan['library'])],check=True)
    assert '(NEEDED)' not in subprocess.check_output(['readelf','-dW',str(library)],text=True)
    assets.append('scan-'+('c' if language=='c' else 'cpp')+'='+str(library))
disk=out/'assets.img';create(disk,assets)
boot=root/'build'/('myos-uefi.img' if args.uefi else 'myos-current.img')
image=out/'frozen-boot.img';before=digest(boot);shutil.copyfile(boot,image)
options=dict(ram=64,boot_image=image,asset_disk=disk)
if args.uefi:options.update(ram=128,boot_bus='ide',firmware=(
    Path('/usr/share/OVMF/OVMF_CODE_4M.fd'),Path('/usr/share/OVMF/OVMF_VARS_4M.fd')))
vm=VM('scan-sdk-uefi' if args.uefi else 'scan-sdk',**options)
try:
    vm.wait('myos> ')
    for name in ('scan-c','scan-cpp'):
        for _ in range(3):
            result=long_command(vm,'elf live '+name)
            assert 'ELF exited with status 88' in result,result
            assert 'native sealed conversion/MPFR/scanf/UTF8/8workers/OOM PASS' in result,result
            assert_live_resources(result)
    print('PASS: native C11/C++17 bases2..36/end/errno/overflow,',count,
          'MPFR decimal double+float vectors/4 rounding modes/long double, '
          'real Mesa wrapper/readonly guard-page edges, '
          'memory scanf widths/types/matching/scansets/UTF8/allocation OOM/recovery/'
          '8 workers/6 launches/exact resources. NOT FILE ABI/renderer/Cobalt.',out,flush=True)
finally:vm.close()
assert before==digest(boot)==digest(image)
