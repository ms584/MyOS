"""Positive pinned SABI/event/argv/stack lifecycle with a genuine second image."""
import argparse,hashlib,json,re,shutil,sys,tempfile
from pathlib import Path
from test_system import ROOT,VM
from test_elf_dynamic import long_command
sys.path.insert(0,str(ROOT/'tools'))
from make_asset_disk import create
p=argparse.ArgumentParser(description=__doc__)
p.add_argument('--provider',type=Path,required=True);p.add_argument('--uefi',action='store_true')
args=p.parse_args()
digest=lambda path:hashlib.sha256(path.read_bytes()).hexdigest()
provider=args.provider.resolve();report=json.loads((provider/'report.json').read_text())
assert digest(provider/'provider.so')==report['provider_sha256'] and report['paired_exit_abi']==1
core=ROOT/'build/engine-evaluation/libcobalt.so'
assert digest(core)==json.loads((ROOT/'docs/PHASE33_COBALT_AUDIT.json').read_text())['library_sha256']
fixture=ROOT/'build/system-tests/starboard-launcher/launch0.so'
assert core.read_bytes()[0x2e2efa:].split(b'\0',1)[0]+b'\0' in fixture.read_bytes()
out=Path(tempfile.mkdtemp(prefix='pair-starboard-',dir=ROOT/'build/renderer-provider-evaluation'))
disk=out/'assets.img';create(disk,['launch='+str(fixture),'trace='+str(ROOT/'build/system-tests/starboard-launcher/launch8.so'),'provider='+str(provider/'provider.so')])
boot=ROOT/'build'/('myos-uefi.img' if args.uefi else 'myos-current.img')
before=digest(boot);frozen=out/'frozen-boot.img';shutil.copyfile(boot,frozen)
options=dict(ram=128,boot_image=frozen,asset_disk=disk)
if args.uefi:options.update(boot_bus='ide',firmware=(Path('/usr/share/OVMF/OVMF_CODE_4M.fd'),Path('/usr/share/OVMF/OVMF_VARS_4M.fd')))
vm=VM('elf-pair-starboard'+('-uefi' if args.uefi else ''),**options)
try:
    vm.wait('myos> ')
    for url in ('',' https://myos.test/a?b=1&c=2'):
        output=long_command(vm,'elf pair-starboard launch provider'+url)
        assert 'USER FAULT' not in output and 'Starboard launcher FAIL' not in output,output
        markers=['SABI: pinned','Starboard launcher init','Starboard launcher constructor',
                 'Starboard launcher 256KiB stack/args PASS','Starboard launcher lifecycle PASS',
                 'Starboard launcher destructor','Starboard launcher finalizer PASS','ELF exited with status 57']
        positions=[output.index(m) for m in markers];assert positions==sorted(positions),output
        for marker in markers:assert output.count(marker)==1,output
        match=re.search(r'ELF pair live: pages reclaimed; heap-before=(\d+) heap-after=(\d+)',output)
        assert match and match[1]==match[2],output
    for mapping in ('',' myos.test=10.0.2.2'):
        output=long_command(vm,'elf cobalt-trace trace provider https://myos.test/a?b=1&c=2'+mapping)
        assert 'Starboard trace argv/TLS/jitless/NULL PASS' in output,output
        assert 'Starboard launcher finalizer PASS' in output and 'ELF exited with status 57' in output,output
        assert 'USER FAULT' not in output and 'PANIC' not in output,output
        match=re.search(r'ELF pair live: pages reclaimed; heap-before=(\d+) heap-after=(\d+)',output)
        assert match and match[1]==match[2],output
    output=long_command(vm,'elf cobalt-trace trace provider https://myos.test/a?b=1&c=2 bad..host=10.0.2.2')
    assert 'ELF pair live: started' not in output and 'Starboard launcher init' not in output,output
    print('PASS: paired positive pinned SABI before init, zero-entry event handler, 2MiB NX/guard stack/256KiB probe, owned readonly argv/URL, Start/Stop/finalizers/relaunch/exact resources; NOT Cobalt.',out,flush=True)
finally:
    vm.close();shutil.copyfile(vm.log,out/'serial.log');assert digest(boot)==before
