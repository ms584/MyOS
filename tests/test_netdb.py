"""Native owned resolver contracts with controlled Ethernet peer; NOT Cobalt."""
import argparse,hashlib,shutil,subprocess,sys,tempfile,time
from pathlib import Path
sys.path.insert(0,str(Path.cwd()/'tests'))
from test_system import ROOT,VM
from test_elf_dynamic import long_command,assert_live_resources,cancel_live
from dns_network_fixture import DnsNetwork
sys.path.insert(0,str(ROOT/'tools'))
from make_asset_disk import create
p=argparse.ArgumentParser(description=__doc__)
p.add_argument('--offline',action='store_true');p.add_argument('--uefi',action='store_true');p.add_argument('--no-entropy',action='store_true')
args=p.parse_args()
base=ROOT/'build/resolver-evaluation';base.mkdir(parents=True,exist_ok=True)
out=Path(tempfile.mkdtemp(prefix='dns-',dir=base))
include=subprocess.check_output(['gcc','-print-file-name=include'],text=True).strip();assets=[]
for mode in range(4):
 obj=out/f'mode{mode}.o';library=out/f'mode{mode}.so'
 subprocess.run(['gcc','-m64','-std=c11','-Os','-fPIC','-ffreestanding','-fno-builtin','-mno-red-zone',
  '-fno-stack-protector','-fno-asynchronous-unwind-tables','-fno-unwind-tables','-nostdinc','-isystem',include,
  '-Iuser/sdk','-Iinclude','-Wall','-Wextra','-Werror','-Wno-unused-function',f'-DMODE={mode}',
  '-c','user/elf-netdb.c','-o',str(obj)],check=True)
 subprocess.run(['ld','-shared','--hash-style=both','-z','now','-z','relro','-z','noexecstack',
  '-e','_start','-o',str(library),str(obj)],check=True)
 assets.append(f'dns{mode}={library}')
disk=out/'assets.img';create(disk,assets)
boot=ROOT/'build'/('myos-uefi.img' if args.uefi else 'myos-current.img');before=hashlib.sha256(boot.read_bytes()).hexdigest()
frozen=out/'boot.img';shutil.copyfile(boot,frozen)
peer=None if args.offline else DnsNetwork()
options=dict(ram=64,boot_image=frozen,asset_disk=disk,cpu='qemu64' if args.no_entropy else 'max',nic=peer.nic if peer else False)
if args.uefi:options.update(boot_bus='ide',firmware=(Path('/usr/share/OVMF/OVMF_CODE_4M.fd'),Path('/usr/share/OVMF/OVMF_VARS_4M.fd')))
vm=VM('native-netdb'+('-offline' if args.offline else '')+('-uefi' if args.uefi else '')+('-no-entropy' if args.no_entropy else ''),**options)
try:
 vm.wait('myos> ')
 if peer:
  result=long_command(vm,'net up');assert 'Network: DHCP configured' in result,result
 mode=0 if args.offline else 3 if args.no_entropy else 1
 for _ in range(2):
  result=long_command(vm,f'elf live dns{mode}');assert 'ELF exited with status 89' in result,result
  assert ('DNS offline numeric/errors PASS' if mode==0 else 'DNS entropy fail-closed PASS' if mode==3 else 'DNS native syscall/concurrent/CNAME/wrong-ID/TCP/timeout/worker-cleanup PASS') in result,result
  assert_live_resources(result)
  if mode==1:assert 'DNS real cross-thread EINTR/owned job cleanup PASS' in result,result
 if mode==1:
  cancel_live(vm,'dns2','DNS blocked workers ready')
  result=long_command(vm,'elf live dns1');assert 'ELF exited with status 89' in result,result;assert_live_resources(result)
  assert any(name=='tcp.test' for name,port in peer.queries),peer.queries
 print('PASS: native owned DNS/ABI/relaunch/resources',vars(args),'NOT Cobalt/Internet.',out,flush=True)
finally:
 if peer:peer.close()
 vm.close();shutil.copyfile(vm.log,out/'serial.log')
assert before==hashlib.sha256(boot.read_bytes()).hexdigest()
