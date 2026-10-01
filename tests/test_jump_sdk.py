"""Exercise real target jumps/stack allocation in MyOS, not signal jumps."""
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
parser=argparse.ArgumentParser();parser.add_argument('--uefi',action='store_true');args=parser.parse_args()
base=root/'build/jump-evaluation';base.mkdir(parents=True,exist_ok=True)
with (base/'last-build.log').open('w') as log,contextlib.redirect_stdout(log):
    build=runpy.run_path(str(root/'tools/build_format_sdk.py'));out=build['out']
    jump=runpy.run_path(str(root/'tools/build_jump_sdk.py'),init_globals={'math_build':build['math']})
registers=out/'register-probe.o'
subprocess.run(['nasm','-f','elf64',str(root/'user/elf-jump-register.asm'),'-o',str(registers)],check=True)
sleep=out/'native-sleep.o'
subprocess.run(['gcc',*build['flags'],'-c',str(root/'user/platform-c11-time.c'),'-o',str(sleep)],check=True)
assets=[]
for language in ('c','c++'):
    obj=out/('jump-fixture-'+language+'.o');library=out/('jump-fixture-'+language+'.so')
    flags=build['flags']+['-D_GNU_SOURCE=1'] if language=='c' else [flag for flag in build['flags'] if flag not in (
        '-std=c11','-frounding-math','-ffp-contract=off')]+[
        '-std=c++17','-nostdinc++','-fno-exceptions','-fno-rtti']
    subprocess.run(['gcc' if language=='c' else 'g++',*flags,'-x',language,
        '-c',str(root/'user/elf-jump-sdk.c'),'-o',str(obj)],check=True)
    subprocess.run(['ld','-shared','--hash-style=both','-Bsymbolic','-z','now','-z','relro',
        '-z','noexecstack','-e','_start','-o',str(library),str(obj),str(registers),str(sleep),
        *map(str,build['objects']),*map(str,build['math']['objects'].values()),str(jump['library'])],check=True)
    assert '(NEEDED)' not in subprocess.check_output(['readelf','-dW',str(library)],text=True)
    assets.append('jump-'+('c' if language=='c' else 'cpp')+'='+str(library))
disk=out/'assets.img';create(disk,assets)
boot=root/'build'/('myos-uefi.img' if args.uefi else 'myos-current.img')
image=out/'frozen-boot.img';before=digest(boot);shutil.copyfile(boot,image)
options=dict(ram=64,boot_image=image,asset_disk=disk)
if args.uefi:options.update(ram=128,boot_bus='ide',firmware=(
    Path('/usr/share/OVMF/OVMF_CODE_4M.fd'),Path('/usr/share/OVMF/OVMF_VARS_4M.fd')))
vm=VM('jump-sdk-uefi' if args.uefi else 'jump-sdk',**options)
try:
    vm.wait('myos> ')
    for name in ('jump-c','jump-cpp'):
        for _ in range(3):
            result=long_command(vm,'elf live '+name)
            assert 'ELF exited with status 89' in result,result
            assert 'native sealed jumps/registers/preemption/nested stacks/8workers PASS' in result,result
            assert_live_resources(result)
    print('PASS: native C11/C++17 sealed setjmp/longjmp/aliases/200-byte guards, '
          '0->1/negative/int extremes, callee registers after timed preemption, '
          'nested contexts/stack allocation/8 workers/6 launches/exact resources. '
          'NOT signal jumps/C++ unwinding/renderer/Cobalt.',out,flush=True)
finally:vm.close()
assert before==digest(boot)==digest(image)
