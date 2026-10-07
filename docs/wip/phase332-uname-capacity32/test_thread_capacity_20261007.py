"""Configured worker capacity with independent native PT_TLS, not Cobalt proof."""
from pathlib import Path
import subprocess,sys,tempfile
from test_system import ROOT,VM
from test_elf_dynamic import long_command,assert_live_resources
sys.path.insert(0,str(ROOT/'tools'))
from make_asset_disk import create
out=Path(tempfile.mkdtemp(prefix='thread-capacity-',dir=ROOT/'build'))
obj=out/'fixture.o';lib=out/'fixture.so'
subprocess.run(['gcc','-m64','-std=c11','-Os','-fPIC','-ffreestanding','-fno-builtin','-mno-red-zone','-fno-stack-protector','-fno-asynchronous-unwind-tables','-fno-unwind-tables','-Iuser/sdk','-Wall','-Wextra','-Werror','-Wno-misleading-indentation','-c','user/elf-thread-capacity.c','-o',str(obj)],check=True)
subprocess.run(['ld','-shared','--hash-style=both','-z','now','-z','relro','-z','noexecstack','-e','_start','-o',str(lib),str(obj)],check=True)
disk=out/'assets.img';create(disk,['capacity='+str(lib)])
uefi='--uefi' in sys.argv
for ram in ((64,256) if uefi else (16,64)):
 options=dict(ram=ram,asset_disk=disk,boot_image=ROOT/'build'/('myos-uefi.img' if uefi else 'myos-current.img'),cpu='max')
 if uefi:options.update(boot_bus='ide',firmware=(Path('/usr/share/OVMF/OVMF_CODE_4M.fd'),Path('/usr/share/OVMF/OVMF_VARS_4M.fd')))
 vm=VM('thread-capacity-'+str(ram)+('-uefi' if uefi else ''),**options)
 try:
  vm.wait('myos> ')
  for _ in range(4):
   result=long_command(vm,'elf live capacity',timeout=90)
   assert 'native configured workers saturation/TLS/reuse/errno PASS' in result and 'ELF exited with status 92' in result,result
   assert_live_resources(result)
 finally:vm.close()
 print('PASS configured held workers/EAGAIN saturation,advertised bound,native TLS independent,3slot-reuse rounds/launch,4relaunch/exact resources',ram,'UEFI' if uefi else 'BIOS',out,flush=True)
