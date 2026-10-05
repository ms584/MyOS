"""Actual Ring3 sparse reservations and real fail-closed entropy device."""
from pathlib import Path
import subprocess,sys,tempfile,time
from test_system import ROOT,VM
from test_shell import type_text
from test_elf_dynamic import long_command,assert_live_resources
sys.path.insert(0,str(ROOT/'tools'))
from make_asset_disk import create
out=Path(tempfile.mkdtemp(prefix='sparse-vm-',dir=ROOT/'build'));assets=[]
for name,defines in [('sparse',[]),('sparsefault',['-DMODE=1']),('sparsecancel',['-DMODE=2']),('sparsenoentropy',['-DNO_ENTROPY'])]:
 obj=out/(name+'.o');lib=out/(name+'.so')
 subprocess.run(['gcc','-m64','-std=c11','-Os','-fPIC','-ffreestanding','-fno-builtin','-mno-red-zone',
  '-fno-stack-protector','-fno-asynchronous-unwind-tables','-fno-unwind-tables','-Wall','-Wextra','-Werror',
  '-Wno-unused-variable','-Wno-unused-function','-Iuser/sdk',*defines,'-c','user/elf-sparse-vm.c','-o',str(obj)],check=True)
 subprocess.run(['ld','-shared','--hash-style=both','-z','now','-z','relro','-z','noexecstack','-e','_start','-o',str(lib),str(obj)],check=True)
 assets.append(name+'='+str(lib))
disk=out/'assets.img';create(disk,assets)
uefi='--uefi' in sys.argv;no_entropy='--no-entropy' in sys.argv
options=dict(ram=64,boot_image=ROOT/'build'/('myos-uefi.img' if uefi else 'myos-current.img'),asset_disk=disk,cpu='qemu64' if no_entropy else 'max')
if uefi:options.update(boot_bus='ide',firmware=(Path('/usr/share/OVMF/OVMF_CODE_4M.fd'),Path('/usr/share/OVMF/OVMF_VARS_4M.fd')))
vm=VM('sparse-vm'+('-uefi' if uefi else '')+('-no-entropy' if no_entropy else ''),**options)
try:
 vm.wait('myos> ')
 for _ in range(3):
  result=long_command(vm,'elf live '+('sparsenoentropy' if no_entropy else 'sparse'),timeout=45)
  assert 'ELF exited with status 89' in result,result
  assert 'real entropy descriptor PASS' in result,result;assert_live_resources(result)
 if not no_entropy:
  result=long_command(vm,'elf live sparsefault',timeout=45)
  assert 'USER FAULT' in result and 'deliberate fault ready' in result,result;assert_live_resources(result)
  offset=len(vm.log.read_text());type_text(vm,'elf live sparsecancel\n')
  vm.wait('sparse reservation cancel ready',45);vm.key('esc')
  end=time.monotonic()+15
  while time.monotonic()<end:
   result=vm.log.read_text()[offset:]
   if result.endswith('myos> '):break
   time.sleep(.05)
  assert 'ELF cancelled; all threads stopped' in result,result;assert_live_resources(result)
  result=long_command(vm,'elf live sparse',timeout=45)
  assert 'ELF exited with status 89' in result,result;assert_live_resources(result)
 print('PASS sparse native32GiB reservation/64GiB alignment/entropy/resources',sys.argv[1:],out,flush=True)
finally:vm.close()
