"""Actual kernel shared frames, not copied MAP_SHARED or browser acceptance."""
from pathlib import Path
import hashlib,subprocess,sys,tempfile
from test_system import ROOT,VM
from test_elf_dynamic import long_command,assert_live_resources
sys.path.insert(0,str(ROOT/'tools'))
from make_asset_disk import create
out=Path(tempfile.mkdtemp(prefix='shared-memory-',dir=ROOT/'build'))
obj=out/'fixture.o';lib=out/'fixture.so'
subprocess.run(['gcc','-m64','-std=c11','-Os','-fPIC','-ffreestanding','-fno-builtin','-mno-red-zone','-fno-stack-protector','-fno-asynchronous-unwind-tables','-fno-unwind-tables','-Iuser/sdk','-Wall','-Wextra','-Werror','-Wno-misleading-indentation','-c','user/elf-shared-memory.c','-o',str(obj)],check=True)
subprocess.run(['ld','-shared','--hash-style=both','-z','now','-z','relro','-z','noexecstack','-e','_start','-o',str(lib),str(obj)],check=True)
disk=out/'assets.img';create(disk,['shm='+str(lib)])
before=hashlib.sha256(disk.read_bytes()).hexdigest()
uefi='--uefi' in sys.argv
# OVMF16MiB failed before any kernel serial output; not an API test result.
for ram in ((64,256) if uefi else (16,64)):
 options=dict(ram=ram,asset_disk=disk,boot_image=ROOT/'build'/('myos-uefi.img' if uefi else 'myos-current.img'),cpu='max')
 if uefi:options.update(boot_bus='ide',firmware=(Path('/usr/share/OVMF/OVMF_CODE_4M.fd'),Path('/usr/share/OVMF/OVMF_VARS_4M.fd')))
 vm=VM('shared-memory-'+str(ram)+('-uefi' if uefi else ''),**options)
 try:
  vm.wait('myos> ')
  for _ in range(4):
   result=long_command(vm,'elf live shm',timeout=90)
   assert 'native shared memory coherence/lifetime/rights/workers PASS' in result,result
   assert 'ELF exited with status 92' in result,result
   assert_live_resources(result)
 finally:vm.close()
 print('PASS native real shared frames,256KiB,fd/alias I/O coherence,read-only ceiling/private copy,unlink/close/exit lifetime,workers/relaunch/exact resources',ram,'UEFI' if uefi else 'BIOS',out,flush=True)
assert hashlib.sha256(disk.read_bytes()).hexdigest()==before
