"""Named SHM is filesystem residency; process aliases must still be reclaimed."""
from pathlib import Path
import hashlib,json,re,shutil,subprocess,sys,tempfile
root=Path.cwd()
sys.path.insert(0,str(root/'tests'));sys.path.insert(0,str(root/'tools'))
from test_system import VM
from test_shell import command
from test_elf_dynamic import long_command
from cobalt_native_resource_inventory import capture
from make_asset_disk import create
out=Path(tempfile.mkdtemp(prefix='named-shm-lifetime-',dir=root/'build'))
include=subprocess.check_output(['gcc','-print-file-name=include'],text=True).strip()
specs=[]
for case in range(4):
 obj=out/('stage%d.o'%case);library=out/('stage%d.so'%case)
 subprocess.run(['gcc','-m64','-std=c11','-Os','-fPIC','-ffreestanding','-fno-builtin',
  '-mno-red-zone','-fno-stack-protector','-fno-asynchronous-unwind-tables','-fno-unwind-tables',
  '-nostdinc','-isystem',include,'-Iuser/sdk','-Iinclude','-Wall','-Wextra','-Werror',
  '-DCASE=%d'%case,'-c',str(root/'user/elf-named-shm-lifetime.c'),'-o',str(obj)],check=True)
 subprocess.run(['ld','-shared','--hash-style=both','-z','now','-z','relro','-z','noexecstack',
  '-e','_start','-o',str(library),str(obj)],check=True)
 specs.append('stage%d='%case+str(library))
disk=out/'assets.img';create(disk,specs)
uefi='--uefi' in sys.argv;image=root/'build'/('myos-uefi.img' if uefi else 'myos-current.img')
digest=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
before_hash=digest(image);frozen=out/'frozen-boot.img';shutil.copyfile(image,frozen)
options=dict(ram=64,asset_disk=disk,boot_image=frozen)
if uefi:options.update(boot_bus='ide',firmware=(Path('/usr/share/OVMF/OVMF_CODE_4M.fd'),Path('/usr/share/OVMF/OVMF_VARS_4M.fd')))
vm=VM('named-shm-lifetime'+('-uefi' if uefi else ''),**options)
def pages():
 result=long_command(vm,'mem');match=re.search(r'Free physical pages: (\d+)',result)
 assert match,result
 return int(match[1])
records=[]
try:
 vm.wait('myos> ');command(vm,'mkdir /dev/shm','myos> ');baseline=pages()
 for iteration in range(3):
  for case,shared in enumerate(((0,1),(1,1),(1,0),(0,0))):
   result=long_command(vm,'elf live stage%d'%case)
   assert 'named SHM creator/alias/unlink lifetime stage PASS' in result and 'ELF exited with status 88' in result,result
   match=re.search(r'ELF resident: free-before=(\d+) free-after=(\d+) shm-before=(\d+) shm-after=(\d+) aliases-before=(\d+) aliases-after=(\d+)',result)
   assert match,result
   values=tuple(map(int,match.groups()))
   assert values[2:4]==shared and values[4:]==(0,0),values
   assert values[0]+values[2]==values[1]+values[3],values
   assert pages()==baseline-shared[1],(baseline,case,values)
   records.append(dict(iteration=iteration,case=case,values=values))
 capture(vm,out,'final')
 final=json.loads((out/'resources-final.json').read_text())
 assert not any(node['shared'] or node['unlinked'] for node in final['nodes'])
 (out/'evidence.json').write_text(json.dumps(dict(records=records,baseline_pages=baseline,uefi=uefi,normal_image_sha256=before_hash),indent=2))
 print('PASS named SHM persists across creator exit; writable aliases reclaimed without munmap; unlink recovers exact pages;12 launches; no residual SHM',out,flush=True)
finally:
 shutil.copyfile(vm.log,out/'serial.txt');vm.close();assert digest(image)==before_hash
