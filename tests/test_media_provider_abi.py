"""Actual optional media-provider ABI validation and rollback in guest."""
from pathlib import Path
import hashlib,json,struct,sys,tempfile,shutil,re,argparse
from test_system import ROOT,VM
from test_elf_dynamic import long_command
sys.path.insert(0,str(ROOT/'tools'))
from make_asset_disk import create
parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('provider',type=Path)
parser.add_argument('--core',type=Path,required=True,help='Native media fixture shared object; link validation only')
parser.add_argument('--boot-image',type=Path)
parser.add_argument('--uefi',action='store_true')
parser.add_argument('--kvm',action='store_true')
args=parser.parse_args()
provider=args.provider.resolve();library=provider/'provider.so'
digest=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
assert digest(library)==json.loads((provider/'report.json').read_text())['provider_sha256']
out=Path(tempfile.mkdtemp(prefix='provider-abi-',dir=ROOT/'build/media-evaluation'))
data=library.read_bytes()
shoff=struct.unpack_from('<Q',data,40)[0];count=struct.unpack_from('<H',data,60)[0]
sections=[struct.unpack_from('<IIQQQQIIQQ',data,shoff+i*64) for i in range(count)]
sym=next(s for s in sections if s[1]==11);strings=sections[sym[6]]
symbols={}
for at in range(sym[4],sym[4]+sym[5],24):
 name=struct.unpack_from('<I',data,at)[0];start=strings[4]+name
 symbols[data[start:data.index(b'\0',start)].decode()]=at
phoff=struct.unpack_from('<Q',data,32)[0];phnum=struct.unpack_from('<H',data,56)[0]
loads=[struct.unpack_from('<IIQQQQQQ',data,phoff+i*56) for i in range(phnum)]
loads=[h for h in loads if h[0]==1]
def address_offset(addr):
 h=next(h for h in loads if h[3]<=addr<h[3]+h[5]);return h[2]+addr-h[3]
variants={}
for name in ('weak','absent','object','writable-marker','wrong-version','short-marker','missing-marker'):
 bad=bytearray(data)
 if name in ('weak','absent','object'):
  at=symbols['SbPlayerSeek']
  if name=='weak':bad[at+4]=0x22
  if name=='object':bad[at+4]=0x11
  if name=='absent':struct.pack_into('<H',bad,at+6,0)
 else:
  at=symbols['MyOSMediaProviderAbi']
  if name=='short-marker':struct.pack_into('<Q',bad,at+16,4)
  elif name=='wrong-version':
   addr=struct.unpack_from('<Q',bad,at+8)[0];struct.pack_into('<Q',bad,address_offset(addr),2)
  elif name=='missing-marker':
   # Rename only this marker, preserving all symbol-table/string bounds.
   start=strings[4]+struct.unpack_from('<I',bad,at)[0];bad[start]=ord('X')
  else:
   writable=next(h for h in loads if h[1]&2)
   struct.pack_into('<Q',bad,at+8,writable[3])
 path=out/(name+'.so');path.write_bytes(bad);variants[name]=path
core=args.core.resolve()
assets=['core='+str(core),'valid='+str(library)]+[k+'='+str(p) for k,p in variants.items()]
disk=out/'assets.img';create(disk,assets)
boot=out/'boot.img';shutil.copyfile(args.boot_image or ROOT/'build'/('myos-uefi.img' if args.uefi else 'myos-current.img'),boot)
before=digest(boot)
if args.kvm:
 sys.path.insert(0,str(ROOT/'docs/wip/native-pmm512/kvm-ata-20261008'))
 from qemu_kvm_optin_20261008 import install
 install()
options=dict(ram=128,boot_image=boot,asset_disk=disk)
if args.uefi:options.update(boot_bus='ide',firmware=(Path('/usr/share/OVMF/OVMF_CODE_4M.fd'),Path('/usr/share/OVMF/OVMF_VARS_4M.fd')))
vm=VM('media-provider-abi'+('-uefi' if args.uefi else ''),**options);outputs={}
def pages():
 m=re.search(r'Free physical pages: (\d+)',long_command(vm,'mem'));assert m;return int(m[1])
try:
 vm.wait('myos> ');baseline=pages()
 for name in ['valid',*variants]:
  output=long_command(vm,'elf pair-link core '+name);outputs[name]=output
  assert 'USER FAULT' not in output and 'PANIC' not in output,output
  assert 'ELF pair live: started' not in output,output
  assert pages()==baseline,(name,output)
  if name in ('valid','missing-marker'):
   assert 'ELF bindings: missing=0' in output,output
   assert 'ELF: invalid executable' not in output and 'unsupported runtime feature' not in output,output
   if name=='valid':assert 'ELF provider: SbPlayerCreate' in output,output
   else:assert 'ELF platform: SbPlayerCreate' in output,output
  else:assert 'invalid executable' in output or 'unsupported runtime feature' in output,output
 output=long_command(vm,'run /bin/hello');assert 'App exited with status 42' in output,output
finally:
 vm.close();shutil.copyfile(vm.log,out/'serial.log');assert digest(boot)==before
(out/'evidence.json').write_text(json.dumps(dict(passed=True,uefi=args.uefi,core_sha256=digest(core),provider_sha256=digest(library),boot_sha256=before,outputs=outputs),indent=2)+'\n')
print('PASS optional media ABI valid/legacy/malformed exports/marker fail-closed and exact rollback',out,flush=True)
