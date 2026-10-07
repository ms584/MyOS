"""Unavailable-media boundary only; NOT playback or full player semantics."""
from pathlib import Path
import subprocess,sys,tempfile
from test_system import ROOT,VM
from test_elf_dynamic import long_command,assert_live_resources
sys.path.insert(0,str(ROOT/'tools'))
from make_asset_disk import create
out=Path(tempfile.mkdtemp(prefix='player-unavailable-',dir=ROOT/'build'))
obj=out/'player.o';lib=out/'player.so'
subprocess.run(['gcc','-m64','-std=c11','-Os','-fPIC','-ffreestanding','-fno-builtin','-mno-red-zone',
 '-msse2','-fno-stack-protector','-fno-asynchronous-unwind-tables','-fno-unwind-tables',
 '-Iuser/starboard-media/include','-Iuser/starboard-system/include','-Wall','-Wextra','-Werror',
 '-c','user/elf-player-unavailable.c','-o',str(obj)],check=True)
subprocess.run(['ld','-shared','--hash-style=both','-z','now','-z','relro','-z','noexecstack',
 '-e','_start','-o',str(lib),str(obj)],check=True)
disk=out/'assets.img';create(disk,['player='+str(lib)])
uefi='--uefi' in sys.argv
options=dict(ram=64,boot_image=ROOT/'build'/('myos-uefi.img' if uefi else 'myos-current.img'),asset_disk=disk)
if uefi:options.update(boot_bus='ide',firmware=(Path('/usr/share/OVMF/OVMF_CODE_4M.fd'),Path('/usr/share/OVMF/OVMF_VARS_4M.fd')))
vm=VM('player-unavailable'+('-uefi' if uefi else ''),**options)
try:
 vm.wait('myos> ')
 for launch in range(3):
  result=long_command(vm,'elf live player')
  assert 'native unavailable player/decode15 binding/' in result and 'ELF exited with status 92' in result,result
  assert_live_resources(result)
finally:vm.close()
print('PASS native unavailable-media15 bindings/ABI/no fake handle/frame/output/callback/relaunch/resources; NOT compressed playback.',out,flush=True)
