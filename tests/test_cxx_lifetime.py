"""Native no-exception allocation/finalization SDK subset, not full C++/Cobalt."""
import argparse
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile

root=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(root/'tests'));sys.path.insert(0,str(root/'tools'))
from test_c11_sdk import digest
from test_system import VM
from test_elf_dynamic import long_command,assert_live_resources,cancel_live
from make_asset_disk import create


def main(uefi=False):
    base=root/'build/cxx-lifetime-evaluation';base.mkdir(parents=True,exist_ok=True)
    out=Path(tempfile.mkdtemp(prefix='native-',dir=base))
    include=subprocess.check_output(['gcc','-print-file-name=include'],text=True).strip()
    flags=['-m64','-Os','-fPIC','-ffreestanding','-fno-builtin','-mno-red-zone',
           '-fno-stack-protector','-fno-asynchronous-unwind-tables','-fno-unwind-tables',
           '-Wall','-Wextra','-Werror','-nostdinc','-isystem',include,'-Iuser/sdk',
           '-D__myos__=1','-U__linux__','-U__linux','-Ulinux','-U__gnu_linux__']
    cxx=['-std=c++17','-nostdinc++','-fno-exceptions','-fno-rtti']
    allocation=out/'allocation.o';finalize=out/'finalize.o'
    subprocess.run(['g++',*cxx,*flags,'-c','user/platform-cxx-allocation.cpp','-o',str(allocation)],cwd=root,check=True)
    subprocess.run(['gcc','-std=c11',*flags,'-c','user/platform-cxx-finalize.c','-o',str(finalize)],cwd=root,check=True)
    specs=[]
    for name,defines in (('lifetime',[]),('oom',['-DCXX_OOM=1']),
                         ('pressure',['-DCXX_PRESSURE=1']),('cancel',['-DCXX_CANCEL=1'])):
        fixture=out/(name+'.o');library=out/(name+'.so')
        subprocess.run(['g++',*cxx,*flags,'-Wno-unused-function','-Wno-unused-variable',*defines,
                        '-c','user/elf-cxx-lifetime.cpp','-o',str(fixture)],cwd=root,check=True)
        # Equal-priority FINI_ARRAY order is deliberately fixture then provider:
        # reverse traversal runs SDK shutdown before the fixture's final check.
        subprocess.run(['ld','-shared','--hash-style=both','-Bsymbolic','-z','now','-z','relro',
                        '-z','noexecstack','-e','_start','-o',str(library),
                        str(fixture),str(allocation),str(finalize)],check=True)
        assert '(NEEDED)' not in subprocess.check_output(['readelf','-dW',str(library)],text=True)
        specs.append(f'{name}={library}')
    disk=out/'assets.img';create(disk,specs)
    boot=root/'build'/('myos-uefi.img' if uefi else 'myos-current.img')
    image=out/'frozen-boot.img';before=digest(boot);shutil.copyfile(boot,image)
    assert before==digest(boot)==digest(image),'Shared image changed during snapshot'
    print('Immutable native boot image:',before,flush=True)
    options=dict(ram=64,boot_image=image,asset_disk=disk)
    if uefi:options.update(ram=128,boot_bus='ide',firmware=(
        Path('/usr/share/OVMF/OVMF_CODE_4M.fd'),Path('/usr/share/OVMF/OVMF_VARS_4M.fd')))
    vm=VM('cxx-lifetime-uefi' if uefi else 'cxx-lifetime',**options)
    try:
        vm.wait('myos> ')
        for _ in range(3):
            result=long_command(vm,'elf live lifetime')
            assert 'ELF exited with status 73' in result,result
            assert result.count('native C++ allocation/destructors/finalizers PASS')==1,result
            assert_live_resources(result)
        result=long_command(vm,'elf live pressure')
        assert 'ELF exited with status 73' in result and 'destructors/finalizers PASS' in result,result
        assert_live_resources(result)
        result=long_command(vm,'elf live oom')
        assert 'USER FAULT' in result and 'vector=6 ' in result,result
        assert 'destructors/finalizers PASS' not in result,result
        assert_live_resources(result)
        cancel_live(vm,'cancel','native C++ allocated workers cancel ready',timeout=15)
        result=long_command(vm,'elf live lifetime')
        assert 'ELF exited with status 73' in result,result
        assert_live_resources(result)
        print('PASS: native compiler new/delete/aligned/sized/nothrow, owned OOM fault, '
              'actual constructor destructors, DSO/reentrant/concurrent once-only finalize, '
              'atexit order, cancel/relaunch/resources. NOT exceptions/full C++/Cobalt.',flush=True)
    finally:vm.close()
    assert digest(image)==before==digest(boot),'Boot image changed'


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--uefi',action='store_true')
    main(parser.parse_args().uefi)
