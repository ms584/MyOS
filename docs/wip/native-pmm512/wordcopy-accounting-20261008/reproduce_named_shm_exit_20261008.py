"""Read-only-main-image diagnostic: named SHM POSIX lifetime vs launcher count.
Expected PANIC is a reproduced defect, NOT a passing user-facing feature.
Disposable RAMFS only, no owner/data disk and no build/image mutations.
"""
from pathlib import Path
import hashlib,json,re,shutil,subprocess,sys,tempfile
root=Path('/home/xytron404/myos');stage=Path(__file__).resolve().parent
sys.path.insert(0,str(root/'tests'));sys.path.insert(0,str(root/'tools'))
from test_system import VM
from test_shell import command,type_text
from make_asset_disk import create
sys.path.insert(0,'/home/xytron404/cobalt-closed-imports-xnVZeH/tests')
from cobalt_native_resource_inventory import capture
out=Path(tempfile.mkdtemp(prefix='named-shm-exit-',dir=root/'build'))
include=subprocess.check_output(['gcc','-print-file-name=include'],text=True).strip()
subprocess.run(['gcc','-m64','-std=c11','-Os','-fPIC','-ffreestanding','-fno-builtin',
 '-mno-red-zone','-fno-stack-protector','-fno-asynchronous-unwind-tables','-fno-unwind-tables',
 '-nostdinc','-isystem',include,'-Iuser/sdk','-Iinclude','-Wall','-Wextra','-Werror',
 '-c',str(stage/'elf-named-shm-exit-20261008.c'),'-o',str(out/'fixture.o')],check=True)
subprocess.run(['ld','-shared','--hash-style=both','-z','now','-z','relro','-z','noexecstack',
 '-e','_start','-o',str(out/'fixture.so'),str(out/'fixture.o')],check=True)
disk=out/'assets.img';create(disk,['named_shm='+str(out/'fixture.so')])
image=root/'build/myos-current.img';digest=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
before_hash=digest(image);frozen=out/'frozen-boot.img';shutil.copyfile(image,frozen)
addresses={}
for line in subprocess.check_output(['nm','-S','build/kernel.elf'],text=True).splitlines():
 fields=line.split()
 if len(fields)==4 and fields[3].split('.lto_priv.')[0]=='free_count':addresses['free_count']=int(fields[0],16)
assert 'free_count' in addresses
vm=VM('named-shm-exit-reproduction',ram=64,asset_disk=disk,boot_image=frozen)
try:
 vm.wait('myos> ');command(vm,'mkdir /dev/shm','myos> ')
 before=int.from_bytes(vm.memory(addresses['free_count'],8),'little')
 type_text(vm,'elf live named_shm\n')
 vm.wait('PANIC: ELF page reclamation',timeout=60)
 assert 'Named SHM intentionally retained; no mappings and fd closed before exit' in vm.log.read_text()
 after=int.from_bytes(vm.memory(addresses['free_count'],8),'little')
 capture(vm,out,'after-panic')
 nodes=json.loads((out/'resources-after-panic.json').read_text())['nodes']
 retained=[node for node in nodes if node['name']=='creator-exit-shm']
 assert len(retained)==1 and retained[0]['shared'] and retained[0]['references']==0 and retained[0]['size']==4096,retained
 assert before-after==1,(before,after)
 result=dict(reproduced_defect=True,before_pages=before,after_pages=after,
  named_shm_backing_pages=1,retained=retained,normal_image_sha256=before_hash,
  claim='single-image global page assertion conflates intentionally retained named SHM backing with process-owned pages; NOT proof of cause of earlier Cobalt panic')
 (out/'reproduction.json').write_text(json.dumps(result,indent=2))
 print('REPRODUCED named SHM exit assertion defect',json.dumps(result),out,flush=True)
finally:
 shutil.copyfile(vm.log,out/'serial.txt');vm.close();assert digest(image)==before_hash
