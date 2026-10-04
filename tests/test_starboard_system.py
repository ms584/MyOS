"""Ring 3 pinned Starboard system/log contracts; not Cobalt acceptance."""
import argparse,hashlib,json,re,shutil,subprocess,sys,tempfile
from pathlib import Path
from test_system import ROOT,VM
from test_elf_dynamic import long_command,assert_live_resources
sys.path.insert(0,str(ROOT/'tools'))
from make_asset_disk import create
p=argparse.ArgumentParser(description=__doc__);p.add_argument('--uefi',action='store_true');p.add_argument('--network',action='store_true');p.add_argument('--no-entropy',action='store_true');p.add_argument('--launcher',action='store_true');args=p.parse_args()
pins=json.loads((ROOT/'docs/PHASE331_ABI_SOURCES.json').read_text())
for header in ('system.h','log.h','time_zone.h'):
 expected=next(item['sha256'] for item in pins['headers'] if item['path']=='starboard/'+header)
 assert hashlib.sha256((ROOT/'user/starboard-system/include/starboard'/header).read_bytes()).hexdigest()==expected
base=ROOT/'build/starboard-system-evaluation';base.mkdir(parents=True,exist_ok=True)
out=Path(tempfile.mkdtemp(prefix='system-',dir=base))
include=subprocess.check_output(['gcc','-print-file-name=include'],text=True).strip()
obj=out/'fixture.o';library=out/'fixture.so'
subprocess.run(['gcc','-m64','-std=c11','-Os','-fPIC','-ffreestanding','-fno-builtin','-mno-red-zone',
 '-fno-stack-protector','-fno-asynchronous-unwind-tables','-fno-unwind-tables','-nostdinc','-isystem',include,
 '-I'+str(ROOT/'user/sdk'),'-I'+str(ROOT/'user/starboard-system/include'),'-I'+str(ROOT/'include'),
 '-Wall','-Wextra','-Werror','-c',str(ROOT/'user/elf-system-services.c'),'-o',str(obj)],check=True)
strings=out/'strings.o'
subprocess.run(['gcc','-m64','-Os','-fPIC','-ffreestanding','-fno-builtin','-mno-red-zone',
 '-fno-stack-protector','-fno-asynchronous-unwind-tables','-fno-unwind-tables',
 '-I'+str(ROOT/'user/sdk'),'-c',str(ROOT/'user/platform-sdk-string.c'),'-o',str(strings)],check=True)
subprocess.run(['ld','-shared','--hash-style=both','-Bsymbolic','-z','now','-z','relro','-z','noexecstack',
 '-e','_start','-o',str(library),str(obj),str(strings)],check=True)
specs=[]
for mode in range(5):
 mode_obj=out/f'fixture{mode}.o';image=out/f'fixture{mode}.so'
 subprocess.run(['gcc','-m64','-std=c11','-Os','-fPIC','-ffreestanding','-fno-builtin','-mno-red-zone',
  '-fno-stack-protector','-fno-asynchronous-unwind-tables','-fno-unwind-tables','-nostdinc','-isystem',include,
  '-I'+str(ROOT/'user/sdk'),'-I'+str(ROOT/'user/starboard-system/include'),'-I'+str(ROOT/'include'),
  '-Wall','-Wextra','-Werror','-DMODE='+str(mode),'-c',str(ROOT/'user/elf-system-services.c'),'-o',str(mode_obj)],check=True)
 subprocess.run(['ld','-shared','--hash-style=both','-Bsymbolic','-z','now','-z','relro','-z','noexecstack',
  '-e','_start','-o',str(image),str(mode_obj),str(strings)],check=True)
 specs.append(f'system{mode}={image}')
if args.launcher:
 headers=out/'include/starboard';headers.mkdir(parents=True,exist_ok=True)
 for name in ('event.h','window.h'):
  data=(ROOT/'build/engine-evaluation/contracts'/name).read_bytes()
  expected=next(item['sha256'] for item in pins['headers'] if item['path']=='starboard/'+name)
  assert hashlib.sha256(data).hexdigest()==expected
  (headers/name).write_bytes(data)
 path_obj=out/'path.o';path_image=out/'path.so'
 subprocess.run(['gcc','-m64','-std=c11','-Os','-fPIC','-ffreestanding','-fno-builtin','-mno-red-zone',
  '-fno-stack-protector','-fno-asynchronous-unwind-tables','-fno-unwind-tables','-nostdinc','-isystem',include,
  '-I'+str(ROOT/'user/starboard-system/include'),'-I'+str(ROOT/'build/engine-evaluation/contracts'),
  '-I'+str(out/'include'),'-I'+str(ROOT/'include'),'-Wall','-Wextra','-Werror','-c',str(ROOT/'user/elf-system-path.c'),'-o',str(path_obj)],check=True)
 subprocess.run(['ld','-shared','--hash-style=both','-z','now','-z','relro','-z','noexecstack','-o',str(path_image),str(path_obj)],check=True)
 specs.append('pathlaunch='+str(path_image))
disk=out/'assets.img';create(disk,specs)
boot=ROOT/'build'/('myos-uefi.img' if args.uefi else 'myos-current.img');frozen=out/'boot.img'
before=hashlib.sha256(boot.read_bytes()).hexdigest();shutil.copyfile(boot,frozen)
options=dict(ram=128,boot_image=frozen,asset_disk=disk,cpu='qemu64' if args.no_entropy else 'max',nic=args.network)
if args.uefi:options.update(boot_bus='ide',firmware=(Path('/usr/share/OVMF/OVMF_CODE_4M.fd'),Path('/usr/share/OVMF/OVMF_VARS_4M.fd')))
vm=VM('starboard-system'+('-uefi' if args.uefi else ''),**options)
try:
 vm.wait('myos> ')
 if args.launcher:
  for _ in range(2):
   result=long_command(vm,'elf starboard pathlaunch');assert 'ELF exited with status 87' in result,result
   assert 'system owned executable path PASS' in result,result;assert_live_resources(result)
 elif args.no_entropy:
  for mode in (3,4,3,4):
   result=long_command(vm,'elf live system'+str(mode));assert 'ELF exited with status 84' in result,result
   assert 'secure random fail-closed PASS' in result,result;assert_live_resources(result)
 elif args.network:
  def check_network(connected):
   result=long_command(vm,'elf live system'+('1' if connected else '2'))
   assert 'network '+('connected' if connected else 'disconnected')+' PASS' in result,result
   assert 'ELF exited with status '+('85' if connected else '86') in result,result;assert_live_resources(result)
  check_network(False)
  result=long_command(vm,'net up');assert 'DHCP' in result,result
  check_network(True)
  network=vm.command('human-monitor-command',{'command-line':'info network'})
  names=re.findall(r'^([^ :\n]+):.*type=nic',network,re.M);assert len(names)==1,network
  vm.command('set_link',{'name':names[0],'up':False});check_network(False)
  vm.command('set_link',{'name':names[0],'up':True});check_network(True)
 else:
  for _ in range(2):
   result=long_command(vm,'elf live system0');assert 'ELF exited with status 83' in result,result
   assert 'typed system/logging/memory/paths/random PASS' in result,result
   assert 'system signal raw PASS' in result,result
   for letter in 'ABCDEFGH':assert result.count(letter*2048)==3,(letter,result)
   assert_live_resources(result)
 print('PASS: pinned system services',vars(args),'relaunch/exact resources. NOT Cobalt.',out,flush=True)
finally:vm.close();shutil.copyfile(vm.log,out/'serial.log')
assert before==hashlib.sha256(boot.read_bytes()).hexdigest()
