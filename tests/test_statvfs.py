"""Native pinned statvfs ABI and checked kernel snapshot; not Cobalt acceptance."""
from pathlib import Path
import hashlib,subprocess,sys,tempfile
from test_system import ROOT,VM
from test_elf_dynamic import long_command,assert_live_resources
sys.path.insert(0,str(ROOT/'tools'))
from make_asset_disk import create
out=Path(tempfile.mkdtemp(prefix='statvfs-',dir=ROOT/'build'))
intrinsics=subprocess.check_output(['gcc','-print-file-name=include'],text=True).strip()
obj=out/'fixture.o';lib=out/'fixture.so'
subprocess.run(['gcc','-m64','-std=c11','-Os','-fPIC','-ffreestanding','-fno-builtin','-mno-red-zone','-fno-stack-protector','-fno-asynchronous-unwind-tables','-fno-unwind-tables','-nostdinc','-isystem',intrinsics,'-Iuser/sdk','-Iuser/starboard-system/include','-Ibuild/engine-evaluation/contracts','-Wall','-Wextra','-Werror','-Wno-misleading-indentation','-c','user/elf-statvfs.c','-o',str(obj)],check=True)
subprocess.run(['ld','-shared','--hash-style=both','-z','now','-z','relro','-z','noexecstack','-e','_start','-o',str(lib),str(obj)],check=True)
disk=out/'assets.img';create(disk,['vfs='+str(lib)])
before=hashlib.sha256(disk.read_bytes()).hexdigest()
uefi='--uefi' in sys.argv
options=dict(ram=64,asset_disk=disk,boot_image=ROOT/'build'/('myos-uefi.img' if uefi else 'myos-current.img'),cpu='max')
if uefi:options.update(boot_bus='ide',firmware=(Path('/usr/share/OVMF/OVMF_CODE_4M.fd'),Path('/usr/share/OVMF/OVMF_VARS_4M.fd')))
vm=VM('statvfs'+('-uefi' if uefi else ''),**options)
try:
 vm.wait('myos> ')
 for _ in range(4):
  result=long_command(vm,'elf live vfs',timeout=45)
  assert 'native pinned statvfs counts/lifetime/bounds/whole-output validation/concurrency PASS' in result,result
  assert 'ELF exited with status 92' in result,result
  assert_live_resources(result)
finally:vm.close()
assert hashlib.sha256(disk.read_bytes()).hexdigest()==before
print('PASS native pinned statvfs ABI, real inode availability/unlink pin, error/whole-output validation, workers and exact resources',out,flush=True)
