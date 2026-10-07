"""Native custom ABI foundation; not yet exported POSIX rename or metadata."""
from pathlib import Path
import subprocess
import sys
import tempfile
from test_system import ROOT,VM
from test_elf_dynamic import long_command,assert_live_resources,cancel_live
sys.path.insert(0,str(ROOT/'tools'))
from make_asset_disk import create
out=Path(tempfile.mkdtemp(prefix='fs-identity-',dir=ROOT/'build'));assets=[]
for name,kind in [('identity',0),('identitycancel',1),('identityfault',2)]:
 obj=out/(name+'.o');lib=out/(name+'.so')
 subprocess.run(['gcc','-m64','-std=c11','-Os','-fPIC','-ffreestanding','-fno-builtin',
  '-mno-red-zone','-fno-stack-protector','-fno-asynchronous-unwind-tables','-fno-unwind-tables',
  '-Wall','-Wextra','-Werror','-Wno-unused-function','-Iinclude',f'-DHOLD_CASE={kind}',
  '-c','user/elf-fs-identity.c','-o',str(obj)],check=True)
 subprocess.run(['ld','-shared','--hash-style=both','-z','now','-z','relro','-z','noexecstack','-e','_start',
  '-o',str(lib),str(obj)],check=True);assets.append(name+'='+str(lib))
disk=out/'assets.img';create(disk,assets)
uefi='--uefi' in sys.argv
options=dict(ram=64,boot_image=ROOT/'build'/('myos-uefi.img' if uefi else 'myos-current.img'),asset_disk=disk)
if uefi:options.update(boot_bus='ide',firmware=(Path('/usr/share/OVMF/OVMF_CODE_4M.fd'),Path('/usr/share/OVMF/OVMF_VARS_4M.fd')))
vm=VM('fs-identity'+('-uefi' if uefi else ''),**options)
try:
 vm.wait('myos> ')
 for launch in range(3):
  result=long_command(vm,'elf live identity');assert 'ELF exited with status 89' in result,result
  assert 'native inode-pinned directory/relative lookup/atomic rename/RW rejection PASS' in result,result
  assert_live_resources(result)
 result=long_command(vm,'elf live identityfault');assert 'native FS identity resources held' in result,result
 assert 'fault' in result.lower() and 'ELF exited with status 89' not in result,result
 assert_live_resources(result)
 cancel_live(vm,'identitycancel','native FS identity resources held')
 result=long_command(vm,'elf live identity');assert 'ELF exited with status 89' in result,result;assert_live_resources(result)
finally:vm.close()
print('PASS native custom FS inode/rename foundation, invalid pointer atomicity, descriptor limits, fault/Escape/relaunch/exact resources.',out,flush=True)
