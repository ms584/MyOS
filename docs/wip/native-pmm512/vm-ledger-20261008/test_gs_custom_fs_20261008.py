"""Frozen experimental guest images; no mutation of the live native build."""
from pathlib import Path
import hashlib,shutil,subprocess,sys,tempfile
root=Path.cwd();stage=Path(__file__).resolve().parent
sys.path.insert(0,str(root/'tests'));sys.path.insert(0,str(root/'tools'))
from test_system import VM
from test_elf_dynamic import long_command,assert_live_resources
from make_asset_disk import create
out=Path(tempfile.mkdtemp(prefix='gs-custom-fs-',dir=root/'build'))
obj=out/'fixture.o';library=out/'fixture.so'
include=subprocess.check_output(['gcc','-print-file-name=include'],text=True).strip()
subprocess.run(['gcc','-m64','-std=c11','-Os','-fPIC','-ffreestanding','-fno-builtin','-mno-red-zone',
 '-fno-stack-protector','-fno-asynchronous-unwind-tables','-fno-unwind-tables','-nostdinc','-isystem',include,
 '-Wall','-Wextra','-Werror','-c',str(stage/'elf-gs-custom-fs-20261008.c'),'-o',str(obj)],check=True)
subprocess.run(['ld','-shared','--hash-style=both','-z','now','-z','relro','-z','noexecstack','-e','_start','-o',str(library),str(obj)],check=True)
assert ' TLS ' not in subprocess.check_output(['readelf','-lW',str(library)],text=True)
disk=out/'assets.img';create(disk,['custom='+str(library)])
uefi='--uefi' in sys.argv
source=root/'build'/('myos-uefi.img' if uefi else 'myos-current.img')
boot=out/'frozen.img';shutil.copyfile(source,boot)
digest=hashlib.sha256(boot.read_bytes()).hexdigest()
options=dict(ram=64,asset_disk=disk,boot_image=boot,cpu='max')
if uefi:options.update(boot_bus='ide',firmware=(Path('/usr/share/OVMF/OVMF_CODE_4M.fd'),Path('/usr/share/OVMF/OVMF_VARS_4M.fd')))
vm=VM('gs-custom-fs'+('-uefi' if uefi else ''),**options)
try:
 vm.wait('myos> ')
 for iteration in range(2):
  result=long_command(vm,'elf live custom',timeout=180)
  assert 'GS errno independent of custom FS/non-TCB/24-worker-reuse PASS' in result and 'ELF exited with status 93' in result,result
  assert_live_resources(result)
  print('PASS GS custom FS8 concurrent/24worker reuse/relaunch/resources',uefi,iteration,flush=True)
finally:
 shutil.copyfile(vm.log,out/'serial.txt');vm.close()
 assert hashlib.sha256(boot.read_bytes()).hexdigest()==digest
print('EVIDENCE',out,flush=True)
