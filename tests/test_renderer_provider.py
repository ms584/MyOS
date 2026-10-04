"""Stage/relocate/discard real provider in MyOS; NOT multi-module execution."""
import argparse
import hashlib
import json
from pathlib import Path
import re
import shutil
import sys
import tempfile
from test_system import VM,ROOT
from test_elf_dynamic import long_command
sys.path.insert(0,str(ROOT/'tools'))
from make_asset_disk import create

parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--provider',type=Path,required=True)
parser.add_argument('--uefi',action='store_true')
parser.add_argument('--ram',type=int,default=64)
parser.add_argument('--expect-oom',action='store_true')
args=parser.parse_args();provider=args.provider.resolve()
assert provider.is_relative_to((ROOT/'build/renderer-provider-evaluation').resolve())
report=json.loads((provider/'report.json').read_text());library=provider/'provider.so'
digest=lambda path:hashlib.sha256(path.read_bytes()).hexdigest()
assert digest(library)==report['provider_sha256'] and report['entry']==0
assert not report['missing_platform_imports'] and not report['native_provider_executed']
out=Path(tempfile.mkdtemp(prefix='native-stage-',dir=provider.parent))
assets=out/'assets.img';create(assets,['provider='+str(library)])
boot=ROOT/'build'/('myos-uefi.img' if args.uefi else 'myos-current.img')
before=digest(boot);frozen=out/'frozen-boot.img';shutil.copyfile(boot,frozen)
options=dict(ram=args.ram,boot_image=frozen,asset_disk=assets)
if args.uefi:options.update(boot_bus='ide',firmware=(Path('/usr/share/OVMF/OVMF_CODE_4M.fd'),
                                                    Path('/usr/share/OVMF/OVMF_VARS_4M.fd')))
vm=VM('renderer-provider-stage'+('-uefi' if args.uefi else '')+
      ('-oom' if args.expect_oom else ''),**options)
def pages():
    output=long_command(vm,'mem');match=re.search(r'Free physical pages: (\d+)',output)
    assert match,output
    return int(match[1])
try:
    vm.wait('myos> ');baseline=pages()
    for _ in range(2):
        output=long_command(vm,'elf link provider')
        assert 'ELF live: started' not in output and 'USER FAULT' not in output,output
        # Record/check rollback even when a success configuration unexpectedly
        # runs out of managed RAM, before asserting the expected outcome.
        assert pages()==baseline,'provider staging page leak'
        if args.expect_oom:
            assert 'ELF bindings: missing=0' not in output and 'out of memory' in output.lower(),output
        else:
            assert 'ELF bindings: missing=0' in output and 'ELF RELRO: protected' in output,output
            assert 'ELF staged image released; not executed' in output,output
            assert 'unsupported runtime feature' not in output,output
    output=long_command(vm,'elf live provider')
    assert 'unsupported runtime feature; not executed' in output and 'ELF live: started' not in output,output
    assert pages()==baseline,'entry-zero rejection page leak'
    print('PASS: real provider '+('OOM rollback' if args.expect_oom else 'relocation/TLS/RELRO staging/discard')+
          ', repeat/page reclamation/entry-zero rejection; NOT provider execution or Cobalt.',out,flush=True)
finally:
    vm.close();shutil.copyfile(vm.log,out/'serial.log');assert digest(boot)==before
