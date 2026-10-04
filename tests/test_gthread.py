"""Native upstream GCC gthread adapter, not a complete C++ library/renderer."""
from pathlib import Path
import argparse
import hashlib
import shutil
import subprocess
import sys
import tempfile
root=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(root/'tests'));sys.path.insert(0,str(root/'tools'))
from test_system import VM
from test_elf_dynamic import long_command,assert_live_resources,cancel_live
from make_asset_disk import create
parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--configuration',type=Path,required=True)
parser.add_argument('--std',choices=('c++98','c++17'),default='c++17')
parser.add_argument('--uefi',action='store_true');args=parser.parse_args()
configuration=args.configuration.resolve()
assert configuration.is_relative_to((root/'build/cxx-target-evaluation').resolve())
base=root/'build/gthread-evaluation';base.mkdir(parents=True,exist_ok=True)
out=Path(tempfile.mkdtemp(prefix='native-',dir=base))
headers=configuration/'build/include'
intrinsics=subprocess.check_output(['gcc','-print-file-name=include'],text=True).strip()
flags=['-m64','-Os','-std='+args.std,'-fPIC','-fvisibility=hidden','-fno-builtin',
       '-mno-red-zone','-fno-stack-protector','-fno-asynchronous-unwind-tables',
       '-fno-unwind-tables','-fno-exceptions','-fno-rtti','-nostdinc','-nostdinc++',
       '-isystem',intrinsics,'-I'+str(headers),'-I'+str(headers/'x86_64-unknown-myos'),
       '-idirafter',str(root/'user/sdk'),'-D__myos__=1','-U__linux__','-U__linux',
       '-Ulinux','-U__gnu_linux__','-Wall','-Wextra','-Werror']
assets=[]
for name,defines in (('threads',[]),('cancel',['-DGTHREAD_CANCEL=1'])):
    obj=out/(name+'.o');library=out/(name+'.so')
    subprocess.run(['g++',*flags,*defines,'-c',str(root/'user/elf-gthread.cpp'),'-o',str(obj)],check=True)
    subprocess.run(['ld','-shared','--hash-style=both','-z','now','-z','relro',
                    '-z','noexecstack','-e','_start','-o',str(library),str(obj)],check=True)
    assert '(NEEDED)' not in subprocess.check_output(['readelf','-dW',str(library)],text=True)
    undefined=subprocess.check_output(['nm','-u',str(library)],text=True)
    assert 'pthread_cancel' not in undefined and ' w ' not in undefined
    assert 'pthread_mutex_lock' in undefined and 'pthread_cond_wait' in undefined
    assets.append(name+'='+str(library))
disk=out/'assets.img';create(disk,assets)
def digest(path):
    with path.open('rb') as stream:return hashlib.file_digest(stream,'sha256').hexdigest()
boot=root/'build'/('myos-uefi.img' if args.uefi else 'myos-current.img')
before=digest(boot);image=out/'frozen-boot.img';shutil.copyfile(boot,image)
options=dict(ram=64,boot_image=image,asset_disk=disk)
if args.uefi:options.update(ram=128,boot_bus='ide',firmware=(
    Path('/usr/share/OVMF/OVMF_CODE_4M.fd'),Path('/usr/share/OVMF/OVMF_VARS_4M.fd')))
vm=VM('gthread-uefi' if args.uefi else 'gthread',**options)
try:
    vm.wait('myos> ')
    for _ in range(3):
        result=long_command(vm,'elf live threads',timeout=45)
        assert 'ELF exited with status 97' in result and 'strong mutex/cond/once/key/timed wait PASS' in result,result
        assert 'USER FAULT' not in result,result
        assert_live_resources(result)
    cancel_live(vm,'cancel','native upstream gthread blocked workers cancel ready',timeout=45)
    result=long_command(vm,'elf live threads',timeout=45)
    assert 'ELF exited with status 97' in result,result
    assert_live_resources(result)
    print('PASS: native upstream gthread 8-worker exclusion/publication/once/key destructors, '
          'recursive mutex/realtime cond deadline, blocked cancellation/relaunch/exact resources; '
          'strong imports, NOT full C++/renderer/Cobalt.',out,flush=True)
finally:vm.close()
assert digest(boot)==before==digest(image)
