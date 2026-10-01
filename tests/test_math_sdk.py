"""Target math prototype in MyOS; MPFR is a test-data generator only."""
import argparse
from pathlib import Path
import runpy
import shutil
import subprocess
import sys

root=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(root/'tests'))
sys.path.insert(0,str(root/'tools'))
from test_c11_sdk import digest
from test_system import VM
from test_elf_dynamic import long_command,assert_live_resources
from make_asset_disk import create
from math_oracle import generate


def main(uefi=False):
    build=runpy.run_path(str(root/'tools/build_math_sdk.py'))
    out=build['out'];flags=build['flags'];objects=list(map(str,build['objects'].values()))
    generate(out/'math-vectors.h')
    source=out/'threads_posix.c'
    shutil.copyfile(root/'vendor/mesa-c11/src/c11/impl/threads_posix.c',source)
    for check in (True,False):
        subprocess.run(['git','apply',*(['--check'] if check else []),
                        '--directory='+str(out.relative_to(root)),'tools/mesa_c11_target.patch'],cwd=root,check=True)
    flags=['-I'+str(build['sdk_include']),*flags,'-I'+str(root/'vendor/mesa-c11/src'),'-I'+str(out),'-DHAVE_PTHREAD','-DHAVE_STRUCT_TIMESPEC']
    for name,source,extra in (('threads',source,['-Wno-unused-function']),
                              ('time',root/'user/platform-c11-time.c',[]),
                              ('fixture',root/'user/elf-sdk-math.c',[])):
        obj=out/(name+'.o')
        subprocess.run(['gcc',*flags,*extra,'-c',str(source),'-o',str(obj)],check=True)
        objects.append(str(obj))
    library=out/'fixture.so'
    subprocess.run(['ld','-shared','--hash-style=both','-Bsymbolic','-z','now','-z','relro',
                    '-z','noexecstack','-e','_start','-o',str(library),*objects],check=True)
    assert '(NEEDED)' not in subprocess.check_output(['readelf','-dW',str(library)],text=True)
    disk=out/'assets.img';create(disk,[f'math={library}'])
    boot=root/'build'/('myos-uefi.img' if uefi else 'myos-current.img')
    image=out/'frozen-boot.img';before=digest(boot);shutil.copyfile(boot,image)
    assert before==digest(boot)==digest(image),'Shared image changed during snapshot'
    print('Immutable native boot image:',before,flush=True)
    options=dict(ram=64,boot_image=image,asset_disk=disk)
    if uefi:options.update(ram=128,boot_bus='ide',firmware=(
        Path('/usr/share/OVMF/OVMF_CODE_4M.fd'),Path('/usr/share/OVMF/OVMF_VARS_4M.fd')))
    vm=VM('sdk-math-uefi' if uefi else 'sdk-math',**options)
    try:
        vm.wait('myos> ')
        for _ in range(3):
            result=long_command(vm,'elf live math')
            assert 'ELF exited with status 72' in result,result
            assert 'native musl math MPFR vectors/rounding/preemption PASS' in result,result
            assert_live_resources(result)
        print('PASS: native target math selected MPFR vectors, IEEE classes/signed zero, '
              'fenv errors/rounding and thread FP preemption/relaunch/resources. '
              'NOT complete libm certification or renderer/Cobalt acceptance.',flush=True)
    finally:vm.close()
    assert digest(image)==before==digest(boot),'Boot image changed'


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--uefi',action='store_true')
    main(parser.parse_args().uefi)
