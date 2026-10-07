"""Native ELF permissions, not Cobalt or general POSIX mprotect completeness."""
from pathlib import Path
import subprocess,sys,tempfile
from test_system import ROOT,VM
from test_elf_dynamic import long_command,assert_live_resources
sys.path.insert(0,str(ROOT/'tools'))
from make_asset_disk import create
out=Path(tempfile.mkdtemp(prefix='image-protect-',dir=ROOT/'build'))
specs=[]
for mode in range(4):
 obj=out/f'image-{mode}.o';lib=out/f'image-{mode}.so'
 subprocess.run(['gcc','-m64','-std=c11','-Os','-fPIC','-ffreestanding','-fno-builtin','-mno-red-zone',
  '-fno-stack-protector','-fno-asynchronous-unwind-tables','-fno-unwind-tables','-Wall','-Wextra','-Werror',
  '-DMODE='+str(mode),'-c','user/elf-image-protect.c','-o',str(obj)],check=True)
 subprocess.run(['ld','-shared','--hash-style=both','-z','now','-z','relro','-z','noexecstack',
  '-e','_start','-o',str(lib),str(obj)],check=True)
 specs.append(f'image{mode}={lib}')
disk=out/'assets.img';create(disk,specs)
uefi='--uefi' in sys.argv
options=dict(ram=64,boot_image=ROOT/'build'/('myos-uefi.img' if uefi else 'myos-current.img'),asset_disk=disk)
if uefi:options.update(boot_bus='ide',firmware=(Path('/usr/share/OVMF/OVMF_CODE_4M.fd'),Path('/usr/share/OVMF/OVMF_VARS_4M.fd')))
vm=VM('image-protect'+('-uefi' if uefi else ''),**options)
try:
 vm.wait('myos> ')
 for mode in (0,1,2,3,0,0):
  result=long_command(vm,'elf live image'+str(mode))
  assert 'ELF RELRO: protected' in result,result
  if mode==0:assert 'native image mprotect rights ceilings/' in result and 'ELF exited with status 95' in result,result
  else:
   expected={1:'0000000000000007',2:'0000000000000005',3:'0000000000000015'}[mode]
   assert 'vector=14 error=0x'+expected in result and 'error: invalid executable' in result,result
  assert_live_resources(result)
finally:vm.close()
print('PASS native ELF mprotect/RW-R-NONE restoration/RELRO/code ceilings/W^X/NX/atomic hole preflight/fault recovery/relaunch/resources.',out,flush=True)
