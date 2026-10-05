from pathlib import Path
import subprocess,sys,tempfile,time
from test_system import ROOT,VM
from test_shell import type_text
from test_elf_dynamic import long_command,assert_live_resources
sys.path.insert(0,str(ROOT/'tools'))
from make_asset_disk import create
out=Path(tempfile.mkdtemp(prefix='file-map-',dir=ROOT/'build'))
data=out/'data.bin';data.write_bytes(bytes((i*37+i//251+19)&255 for i in range(12295)))
assets=['mapdata='+str(data)]
for name,mode in [('filemap',0),('mapfault',1),('mapcancel',2)]:
 obj=out/(name+'.o');lib=out/(name+'.so')
 subprocess.run(['gcc','-m64','-std=c11','-Os','-fPIC','-ffreestanding','-fno-builtin','-mno-red-zone',
  '-fno-stack-protector','-fno-asynchronous-unwind-tables','-fno-unwind-tables','-Wall','-Wextra','-Werror',
  '-Wno-unused-function','-Wno-unused-variable','-DMODE='+str(mode),'-c','user/elf-file-map.c','-o',str(obj)],check=True)
 subprocess.run(['ld','-shared','--hash-style=both','-z','now','-z','relro','-z','noexecstack','-e','_start','-o',str(lib),str(obj)],check=True)
 assets.append(name+'='+str(lib))
disk=out/'assets.img';create(disk,assets)
uefi='--uefi' in sys.argv
options=dict(ram=64,boot_image=ROOT/'build'/('myos-uefi.img' if uefi else 'myos-current.img'),asset_disk=disk,cpu='max')
if uefi:options.update(boot_bus='ide',firmware=(Path('/usr/share/OVMF/OVMF_CODE_4M.fd'),Path('/usr/share/OVMF/OVMF_VARS_4M.fd')))
vm=VM('file-map'+('-uefi' if uefi else ''),**options)
try:
 vm.wait('myos> ')
 for _ in range(3):
  result=long_command(vm,'elf live filemap',timeout=45)
  assert 'ELF exited with status 90' in result,result
  assert 'immutable file map bytes/offset/lifetime/rights/EOF/partial/fixed PASS' in result,result
  assert_live_resources(result)
 result=long_command(vm,'elf live mapfault',timeout=45)
 assert 'USER FAULT' in result and 'readonly deliberate fault ready' in result,result
 assert_live_resources(result)
 offset=len(vm.log.read_text());type_text(vm,'elf live mapcancel\n')
 vm.wait('file map cancel ready',45);vm.key('esc')
 end=time.monotonic()+15
 while time.monotonic()<end:
  result=vm.log.read_text()[offset:]
  if result.endswith('myos> '):break
  time.sleep(.05)
 assert 'ELF cancelled; all threads stopped' in result,result
 assert_live_resources(result)
 result=long_command(vm,'elf live filemap',timeout=45)
 assert 'ELF exited with status 90' in result,result;assert_live_resources(result)
 print('PASS native immutable file mapping and resource cleanup',sys.argv[1:],out,flush=True)
finally:vm.close()
