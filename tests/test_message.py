"""Private native prototype proof; not full UDP/ancillary message support."""
from pathlib import Path
import subprocess
import sys
import tempfile
from test_system import ROOT, VM
from test_elf_dynamic import long_command, assert_live_resources
sys.path.insert(0,str(ROOT/'tools'))
from make_asset_disk import create
out=Path(tempfile.mkdtemp(prefix='message-',dir=ROOT/'build'))
obj=out/'fixture.o';lib=out/'fixture.so'
subprocess.run(['gcc','-m64','-std=c11','-Os','-fPIC','-ffreestanding','-fno-builtin',
 '-mno-red-zone','-fno-stack-protector','-fno-asynchronous-unwind-tables',
 '-fno-unwind-tables','-Wall','-Wextra','-Werror','-c','user/elf-message.c','-o',str(obj)],check=True)
subprocess.run(['ld','-shared','--hash-style=both','-z','now','-z','relro',
 '-z','noexecstack','-e','_start','-o',str(lib),str(obj)],check=True)
disk=out/'assets.img';create(disk,['message='+str(lib)])
uefi='--uefi' in sys.argv
options=dict(ram=64,boot_image=ROOT/'build'/('myos-uefi.img' if uefi else 'myos-current.img'),asset_disk=disk)
if uefi:options.update(boot_bus='ide',firmware=(Path('/usr/share/OVMF/OVMF_CODE_4M.fd'),Path('/usr/share/OVMF/OVMF_VARS_4M.fd')))
vm=VM('message'+('-uefi' if uefi else ''),**options)
try:
 vm.wait('myos> ')
 for _ in range(4):
  result=long_command(vm,'elf live message')
  assert 'native message stream/pin/scatter/peek/close-reuse PASS' in result,result
  assert 'ELF exited with status 88' in result,result
  assert_live_resources(result)
finally:vm.close()
print('PASS: PRIVATE native stream sendmsg/recvmsg subset, four relaunches/exact resources.',out,flush=True)
