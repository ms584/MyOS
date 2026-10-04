"""Native kernel-owned self signals, handler contexts/faults and real SIGABRT."""
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
def digest(path):
    with path.open('rb') as stream:return hashlib.file_digest(stream,'sha256').hexdigest()
parser=argparse.ArgumentParser();parser.add_argument('--uefi',action='store_true');args=parser.parse_args()
base=root/'build/signal-evaluation';base.mkdir(parents=True,exist_ok=True)
out=Path(tempfile.mkdtemp(prefix='delivery-',dir=base))
include=subprocess.check_output(['gcc','-print-file-name=include'],text=True).strip()
flags=['-m64','-Os','-fPIC','-ffreestanding','-fno-builtin','-mno-red-zone',
       '-fno-stack-protector','-fno-asynchronous-unwind-tables','-fno-unwind-tables',
       '-nostdinc','-isystem',include,'-I'+str(root/'user/sdk'),'-D__myos__=1',
       '-U__linux__','-U__linux','-Ulinux','-U__gnu_linux__','-Wall','-Wextra','-Werror',
       '-Wno-unused-function','-Wno-unused-variable','-Wno-missing-field-initializers']
probe=out/'probe.o'
subprocess.run(['nasm','-f','elf64',str(root/'user/signal-probe.asm'),'-o',str(probe)],check=True)
assets=[]
for name,compiler,case in [('c','gcc',0),('cpp','g++',0),*[(f'case{i}','gcc',i) for i in range(1,13)]]:
    extra=['-std=c11'] if compiler=='gcc' else ['-std=c++17','-nostdinc++','-fno-exceptions','-fno-rtti']
    obj=out/(name+'.o');library=out/(name+'.so')
    subprocess.run([compiler,*flags,*extra,'-DSIGNAL_CASE='+str(case),'-x',
                    'c' if compiler=='gcc' else 'c++','-c',str(root/'user/elf-signal-delivery.c'),'-o',str(obj)],check=True)
    subprocess.run(['ld','-shared','-Bsymbolic','--hash-style=both','-z','now','-z','relro',
                    '-z','noexecstack','-e','_start','-o',str(library),str(obj),str(probe)],check=True)
    assert '(NEEDED)' not in subprocess.check_output(['readelf','-dW',str(library)],text=True)
    assets.append(f'{name}={library}')
disk=out/'assets.img';create(disk,assets)
boot=root/'build'/('myos-uefi.img' if args.uefi else 'myos-current.img')
before=digest(boot);image=out/'frozen-boot.img';shutil.copyfile(boot,image)
options=dict(ram=64,boot_image=image,asset_disk=disk)
if args.uefi:options.update(ram=128,boot_bus='ide',firmware=(
    Path('/usr/share/OVMF/OVMF_CODE_4M.fd'),Path('/usr/share/OVMF/OVMF_VARS_4M.fd')))
vm=VM('signal-delivery-uefi' if args.uefi else 'signal-delivery',**options)
try:
    vm.wait('myos> ')
    for _ in range(3):
        for name in ('c','cpp'):
            result=long_command(vm,'elf live '+name,timeout=30)
            assert 'ELF exited with status 90' in result,result
            assert 'native self signals/pending/nesting/fault/context PASS' in result,result
            assert 'USER FAULT' not in result and 'USER SIGNAL' not in result,result
            assert_live_resources(result)
        for case in (1,2,3,4,5,6):
            result=long_command(vm,'elf live case'+str(case),timeout=30)
            assert 'USER SIGNAL pid=' in result and (' signal=10' if case==6 else ' signal=6') in result,result
            assert 'ELF exited with status' not in result and 'USER FAULT' not in result,result
            assert ('SIGABRT handler entered' in result)==(case in (2,5)),result
            assert_live_resources(result)
        result=long_command(vm,'elf live case7',timeout=30)
        assert 'SIGABRT handler entered' in result and 'ELF exited with status 93' in result,result
        assert 'USER SIGNAL' not in result and 'USER FAULT' not in result,result
        assert_live_resources(result)
        result=long_command(vm,'elf live case10',timeout=30)
        assert 'native forged sigreturn rejected PASS' in result and 'ELF exited with status 94' in result,result
        assert_live_resources(result)
        result=long_command(vm,'elf live case11',timeout=30)
        assert 'native handler pthread_exit/slot reuse PASS' in result and 'ELF exited with status 95' in result,result
        assert_live_resources(result)
        result=long_command(vm,'elf live case12',timeout=30)
        assert 'USER SIGNAL pid=' in result and ' signal=11' in result,result
        assert 'USER FAULT' not in result and 'ELF exited with status' not in result,result
        assert_live_resources(result)
        for case in (8,9):cancel_live(vm,'case'+str(case),'signal handler cancellation ready')
    print('PASS: native C/C++ self raise/action/masks/coalescing/IGNORE discard, '
          'NODEFER/RESETHAND/nested/preempted handlers,15GP/DF/16XMM/x87/MXCSR '
          'restoration, repaired readonly fault, inherited worker masks, '
          'actual SIGABRT default/handler/ignore/block/worker/nonreturn, forged '
          'live/fresh forged return rejection,16 handler pthread_exit slot reuses, '
          'bounded nesting exhaustion,36 launches/6handler cancels/exact resources. '
          'No SA_SIGINFO/alt-stack/restart/cross-thread/pause/Cobalt acceptance.',out,flush=True)
finally:vm.close()
assert before==digest(boot)==digest(image)
