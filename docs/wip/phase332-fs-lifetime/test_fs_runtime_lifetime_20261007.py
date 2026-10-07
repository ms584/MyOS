"""Native runtime lifetime proof; not actual Cobalt or blocking-lock proof."""
from pathlib import Path
import subprocess,sys,tempfile
from test_system import ROOT,VM
from test_elf_dynamic import long_command,assert_live_resources
sys.path.insert(0,str(ROOT/'tools'))
from make_asset_disk import create
out=Path(tempfile.mkdtemp(prefix='fs-runtime-',dir=ROOT/'build'))
obj=out/'runtime.o';lib=out/'runtime.so'
include=subprocess.check_output(['gcc','-print-file-name=include'],text=True).strip()
subprocess.run(['gcc','-m64','-std=c11','-Os','-fPIC','-ffreestanding','-fno-builtin','-mno-red-zone',
 '-fno-stack-protector','-fno-asynchronous-unwind-tables','-fno-unwind-tables','-nostdinc',
 '-isystem',include,'-Iuser/sdk','-Iinclude','-Wall','-Wextra','-Werror',
 '-c','user/elf-fs-runtime-lifetime.c','-o',str(obj)],check=True)
subprocess.run(['ld','-shared','--hash-style=both','-z','now','-z','relro','-z','noexecstack',
 '-e','_start','-o',str(lib),str(obj)],check=True)
disk=out/'assets.img';create(disk,['runtime='+str(lib)])
uefi='--uefi' in sys.argv
options=dict(ram=64,boot_image=ROOT/'build'/('myos-uefi.img' if uefi else 'myos-current.img'),asset_disk=disk)
if uefi:options.update(boot_bus='ide',firmware=(Path('/usr/share/OVMF/OVMF_CODE_4M.fd'),Path('/usr/share/OVMF/OVMF_VARS_4M.fd')))
vm=VM('fs-runtime'+('-uefi' if uefi else ''),**options)
try:
 vm.wait('myos> ')
 for launch in range(4):
  result=long_command(vm,'elf live runtime')
  assert 'native runtime inode DIR/' in result and 'ELF exited with status 88' in result,result
  assert_live_resources(result)
finally:vm.close()
print('PASS native runtime DIR inode/rename/relative traversal/retained stream/close-reuse/typed fcntl whole-output guard/readonly assets/relaunch/exact resources; NOT blocking lock/actual Cobalt.',out,flush=True)
