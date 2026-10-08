from pathlib import Path
import subprocess,sys,tempfile,shutil,hashlib
root=Path.cwd();stage=Path(__file__).resolve().parent
sys.path.insert(0,str(root/'tests'));sys.path.insert(0,str(root/'tools'))
from test_system import VM
from test_elf_dynamic import long_command,assert_live_resources
from make_asset_disk import create
out=Path(tempfile.mkdtemp(prefix='gs-errno-',dir=root/'build'));specs=[]
include=subprocess.check_output(['gcc','-print-file-name=include'],text=True).strip()
for case in range(2):
 obj=out/('case%d.o'%case);lib=out/('case%d.so'%case)
 subprocess.run(['gcc','-m64','-std=c11','-Os','-fPIC','-ffreestanding','-fno-builtin','-mno-red-zone','-fno-stack-protector',
 '-fno-asynchronous-unwind-tables','-fno-unwind-tables','-nostdinc','-isystem',include,'-Iuser/sdk','-Wall','-Wextra','-Werror',
 '-Wno-misleading-indentation','-DWITH_TLS=%d'%case,'-c',str(stage/'elf-gs-errno-20261008.c'),'-o',str(obj)],check=True)
 subprocess.run(['ld','-shared','--hash-style=both','-z','now','-z','relro','-z','noexecstack','-e','_start','-o',str(lib),str(obj)],check=True)
 specs.append('case%d='%case+str(lib))
disk=out/'assets.img';create(disk,specs);uefi='--uefi' in sys.argv
image=root/'build'/('myos-uefi.img' if uefi else 'myos-current.img');before=hashlib.sha256(image.read_bytes()).hexdigest()
frozen=out/'frozen.img';shutil.copyfile(image,frozen)
options=dict(ram=64,asset_disk=disk,boot_image=frozen,cpu='max')
if uefi:options.update(boot_bus='ide',firmware=(Path('/usr/share/OVMF/OVMF_CODE_4M.fd'),Path('/usr/share/OVMF/OVMF_VARS_4M.fd')))
vm=VM('gs-errno'+('-uefi' if uefi else ''),**options)
try:
 vm.wait('myos> ')
 for case in range(2):
  for iteration in range(2):
   result=long_command(vm,'elf live case%d'%case,timeout=120)
   assert 'GS errno oracle/isolation/preemption/48-worker-reuse PASS' in result,result
   assert 'ELF exited with status 89' in result,result
   assert_live_resources(result)
   print('PASS GS errno oracle/no-TLS-or-ELF-TLS/48worker-reuse',uefi,case,iteration,flush=True)
   for line in result.splitlines():
    if 'GS errno raw TSC' in line:print(line,flush=True)
finally:
 shutil.copyfile(vm.log,out/'serial.txt');vm.close();assert hashlib.sha256(image.read_bytes()).hexdigest()==before
print('EVIDENCE',out,flush=True)
