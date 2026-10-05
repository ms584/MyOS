"""Native targeted signals/waits/lifetime subset; not full POSIX or Cobalt."""
import hashlib
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
root=Path.cwd()
sys.path.insert(0,str(root/'tests'));sys.path.insert(0,str(root/'tools'))
from test_system import VM
from test_elf_dynamic import long_command,assert_live_resources
from make_asset_disk import create
out=Path(tempfile.mkdtemp(prefix='cross-',dir=root/'build'))
include=subprocess.check_output(['gcc','-print-file-name=include'],text=True).strip()
assets=[]
for mode,name in enumerate(('cross','main-exit','default-worker','default-main')):
 obj=out/(name+'.o');library=out/(name+'.so')
 subprocess.run(['gcc','-m64','-std=c11','-Os','-fPIC','-ffreestanding','-fno-builtin',
  '-mno-red-zone','-fno-stack-protector','-fno-asynchronous-unwind-tables','-fno-unwind-tables',
  '-nostdinc','-isystem',include,'-I'+str(root/'user/sdk'),'-Wall','-Wextra','-Werror',
  '-DEXIT_MAIN_CASE='+str(mode),'-c',sys.argv[1],'-o',str(obj)],check=True)
 subprocess.run(['ld','-shared','--hash-style=both','-z','now','-z','relro','-z','noexecstack',
  '-e','_start','-fini','finalizer','-o',str(library),str(obj)],check=True)
 assets.append(name+'='+str(library))
disk=out/'assets.img';create(disk,assets)
uefi='--uefi' in sys.argv[2:]
boot=root/'build'/('myos-uefi.img' if uefi else 'myos-current.img');before=hashlib.sha256(boot.read_bytes()).hexdigest()
image=out/'frozen-boot.img';shutil.copyfile(boot,image)
options=dict(ram=64,boot_image=image,asset_disk=disk)
if uefi:options.update(ram=128,boot_bus='ide',firmware=(
 Path('/usr/share/OVMF/OVMF_CODE_4M.fd'),Path('/usr/share/OVMF/OVMF_VARS_4M.fd')))
vm=VM('cross-signals-uefi' if uefi else 'cross-signals',**options)
try:
 vm.wait('myos> ')
 for _ in range(4):
  result=long_command(vm,'elf live cross',timeout=40)
  assert 'ELF exited with status 96' in result,result
  assert 'native targeted signals/masks/pause/sleep/sem/pipe/poll/epoll/pthread waits PASS' in result,result
  assert 'USER FAULT' not in result and 'USER SIGNAL' not in result,result
  assert 'cross signal finalizer called' in result,result
  assert_live_resources(result)
  result=long_command(vm,'elf live main-exit',timeout=40)
  assert 'ELF exited with status 0' in result,result
  assert 'native exited main identity/signal liveness PASS' in result,result
  assert 'USER FAULT' not in result and 'USER SIGNAL' not in result,result
  assert 'cross signal finalizer called' in result,result
  assert_live_resources(result)
  for name in ('default-worker','default-main'):
   result=long_command(vm,'elf live '+name,timeout=40)
   assert 'native targeted default termination ready' in result,result
   assert 'USER SIGNAL pid=' in result and ' signal=15' in result,result
   assert 'ELF exited with status' not in result and 'cross signal finalizer called' not in result,result
   assert 'USER FAULT' not in result,result
   assert_live_resources(result)
 print('PASS: native targeted signals/waits/lifetime/default termination,16 launches/exact resources; NOT Cobalt/full POSIX.',out)
finally:
 vm.close();shutil.copyfile(vm.log,out/'serial.log')
assert before==hashlib.sha256(boot.read_bytes()).hexdigest()
