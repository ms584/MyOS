"""Actual pinned core/provider coexistence, biased linking and rollback; no execution."""
import argparse
import hashlib
import json
from pathlib import Path
import re
import shutil
import struct
import sys
import tempfile
from test_system import VM,ROOT
from test_elf_dynamic import long_command
sys.path.insert(0,str(ROOT/'tools'))
from make_asset_disk import create

p=argparse.ArgumentParser(description=__doc__)
p.add_argument('--provider',type=Path,required=True)
p.add_argument('--core',type=Path,default=ROOT/'build/engine-evaluation/libcobalt.so')
p.add_argument('--uefi',action='store_true')
p.add_argument('--ram',type=int,default=256)
p.add_argument('--expect-oom',action='store_true')
p.add_argument('--expect-second-module',action='store_true',help='Require observed second-image allocation before OOM')
args=p.parse_args()
if args.expect_second_module and not args.expect_oom:p.error('--expect-second-module requires --expect-oom')
digest=lambda path:hashlib.sha256(path.read_bytes()).hexdigest()
provider=args.provider.resolve();library=provider/'provider.so'
assert provider.is_relative_to((ROOT/'build/renderer-provider-evaluation').resolve())
report=json.loads((provider/'report.json').read_text())
assert digest(library)==report['provider_sha256']
assert digest(args.core)==json.loads((ROOT/'docs/PHASE33_COBALT_AUDIT.json').read_text())['library_sha256']

def headers(data):
    offset=struct.unpack_from('<Q',data,32)[0]
    count=struct.unpack_from('<H',data,56)[0]
    return [(offset+i*56,struct.unpack_from('<IIQQQQQQ',data,offset+i*56)) for i in range(count)]

out=Path(tempfile.mkdtemp(prefix='pair-',dir=ROOT/'build/renderer-provider-evaluation'))
data=library.read_bytes();loads=[h for _,h in headers(args.core.read_bytes()) if h[0]==1]
end=max(h[3]+h[6] for h in loads)
expected_bias=0x8000000000+((end+4095)&~4095)+4096
assets=['core='+str(args.core),'provider='+str(library)]
if not args.expect_oom:
    # Real provider mutations, not fake renderer implementations.
    wide=bytearray(data)
    last=max((item for item in headers(data) if item[1][0]==1),key=lambda item:item[1][3])
    struct.pack_into('<Q',wide,last[0]+40,128*1024*1024)
    path=out/'wide.so';path.write_bytes(wide);assets.append('wide='+str(path))
    # Find the real exported getter via the bounded ELF dynamic symbol section.
    shoff=struct.unpack_from('<Q',data,40)[0];count=struct.unpack_from('<H',data,60)[0]
    sections=[struct.unpack_from('<IIQQQQIIQQ',data,shoff+i*64) for i in range(count)]
    sym=next(s for s in sections if s[1]==11);strings=sections[sym[6]]
    bad=bytearray(data);found=False
    for at in range(sym[4],sym[4]+sym[5],24):
        name=struct.unpack_from('<I',data,at)[0];start=strings[4]+name
        if data[start:data.index(b'\0',start)]==b'SbGetEglInterface':
            bad[at+4]=0x22;found=True;break
    assert found
    path=out/'weak-getter.so';path.write_bytes(bad);assets.append('weakgetter='+str(path))
    dynamic=next(h for _,h in headers(data) if h[0]==2)
    tags=dict(struct.unpack_from('<qQ',data,at) for at in range(dynamic[2],dynamic[2]+dynamic[5],16))
    owner=next(h for _,h in headers(data) if h[0]==1 and h[3]<=tags[7]<h[3]+h[5])
    rela=owner[2]+tags[7]-owner[3];bad=bytearray(data);found=False
    for at in range(rela,rela+tags[8],24):
        if struct.unpack_from('<Q',data,at+8)[0]&0xffffffff==8:
            struct.pack_into('<q',bad,at+16,-4096);found=True;break
    assert found
    path=out/'cross-base.so';path.write_bytes(bad);assets.append('crossbase='+str(path))

disk=out/'assets.img';create(disk,assets)
boot=ROOT/'build'/('myos-uefi.img' if args.uefi else 'myos-current.img')
before=digest(boot);frozen=out/'frozen-boot.img';shutil.copyfile(boot,frozen)
options=dict(ram=args.ram,boot_image=frozen,asset_disk=disk)
if args.uefi:options.update(boot_bus='ide',firmware=(Path('/usr/share/OVMF/OVMF_CODE_4M.fd'),Path('/usr/share/OVMF/OVMF_VARS_4M.fd')))
vm=VM('elf-pair'+('-uefi' if args.uefi else '')+('-oom' if args.expect_oom else ''),**options)
def pages():
    output=long_command(vm,'mem');match=re.search(r'Free physical pages: (\d+)',output)
    assert match,output
    return int(match[1])
try:
    vm.wait('myos> ');baseline=pages()
    for _ in range(2):
        output=long_command(vm,'elf pair-link core provider')
        assert pages()==baseline,'paired image page leak'
        assert 'ELF live: started' not in output and 'USER FAULT' not in output,output
        if args.expect_oom:
            assert 'out of memory' in output.lower(),output
            if args.expect_second_module:assert 'ELF pair mapping module: 1' in output,output
        else:
            match=re.search(r'ELF pair provider bias: (0x[0-9A-Fa-f]+)',output)
            assert match and int(match[1],16)==expected_bias,output
            assert 'ELF provider: SbGetEglInterface' in output and 'ELF provider: SbGetGlesInterface' in output,output
            assert 'ELF bindings: missing=0' in output and 'ELF bindings: missing=41' in output,output
            assert output.count('ELF RELRO: protected')==2,output
            assert 'ELF missing: SbGetEglInterface' not in output and 'ELF missing: SbGetGlesInterface' not in output,output
            assert 'ELF pair images released; not executed' in output and 'unsupported runtime feature' in output,output
    if not args.expect_oom:
        for command,marker in [('elf pair-link core wide','invalid executable'),
                               ('elf pair-link core weakgetter','invalid executable'),
                               ('elf pair-link core crossbase','invalid executable'),
                               ('elf pair-link provider provider','unsupported runtime feature')]:
            output=long_command(vm,command)
            assert marker in output and 'ELF live: started' not in output,output
            assert pages()==baseline,command+' leaked pages'
    output=long_command(vm,'run /bin/hello')
    assert 'App exited with status 42' in output,output
    print('PASS: actual pair '+('OOM' if args.expect_oom else 'biased exports/relocations/TLS/RELRO/missing-import and adversarial')+
          ' rollback/repeat/pages/heap kernel checks/shell recovery; NOT execution.',out,flush=True)
finally:
    vm.close();shutil.copyfile(vm.log,out/'serial.log');assert digest(boot)==before
