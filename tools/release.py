"""One MyOS build/test/package workflow. Default: validate and package a release."""
import argparse
from hashlib import sha256
import json
from pathlib import Path
import shutil
import subprocess
import sys
import zipfile

ROOT=Path(__file__).resolve().parents[1]
ARTIFACTS=['myos-bios.img','myos-uefi.img','myos-usb.img']
RECORD=ROOT/'build/release-validation.json'
def run(*args):
    print('$ '+' '.join(map(str,args)),flush=True)
    subprocess.run(args,cwd=ROOT,check=True)
def output(*args):return subprocess.check_output(args,cwd=ROOT,text=True).strip()
def digest(path):return sha256(path.read_bytes()).hexdigest()
def sources():
    paths=subprocess.check_output(['git','ls-files','-z','--cached','--others','--exclude-standard'],cwd=ROOT).split(b'\0')
    state=sha256()
    for name in sorted(set(paths)-{b''}):
        path=ROOT/name.decode()
        if not path.is_file():raise SystemExit(f'Missing source: {name!r}')
        state.update(name+b'\0'+path.read_bytes()+b'\0')
    return state.hexdigest()
def hashes():return {name:digest(ROOT/'build'/name) for name in ARTIFACTS}
def data_hashes():
    # User save disks live outside the ignored test fixtures. They are never
    # generated, cleaned or reformatted by this workflow.
    return {str(p):digest(p) for p in (ROOT/'build').glob('*data*.img') if p.is_file()}
def versions():
    result={}
    for tool,args in [('gcc',['--version']),('ld',['--version']),('objcopy',['--version']),
                      ('nasm',['-v']),('qemu-system-x86_64',['--version']),('python3',['--version']),
                      ('fsck.fat',['--help']),('mcopy',['-V'])]:
        if not shutil.which(tool):raise SystemExit(f'Missing dependency: {tool}; see docs/RELEASE.md')
        p=subprocess.run([tool,*args],text=True,stdout=subprocess.PIPE,stderr=subprocess.STDOUT)
        result[tool]=p.stdout.splitlines()[0]
    result['gnu-efi']=output('dpkg-query','-W','-f=${Version}','gnu-efi')
    for name in ('OVMF_CODE_4M.fd','OVMF_VARS_4M.fd'):
        path=Path('/usr/share/OVMF')/name
        if not path.is_file():raise SystemExit(f'Missing firmware: {path}')
        result[name]=digest(path)
    return result
def build():
    run(sys.executable,'tools/package_bios.py')
    run(sys.executable,'tools/build_uefi.py')
    run(sys.executable,'tools/package_usb.py')
def test():
    source=sources();saved=data_hashes();toolchain=versions()
    tests=['tests/test_system.py','tests/test_bios_package.py']
    for script in tests:run(sys.executable,script)
    run(sys.executable,'tools/build_uefi.py');run(sys.executable,'tools/package_usb.py')
    for script in ['tests/test_firmware.py','tests/test_usb_mouse.py','tests/test_usb_storage.py','tests/test_usb_bot_protocol.py','tests/test_xhci_ports.py','tests/test_release_tools.py','tests/test_demo_package.py']:
        run(sys.executable,script);tests.append(script)
    before=hashes()
    build()
    if before!=hashes():raise SystemExit('Build reproducibility failed: image bytes changed')
    if saved!=data_hashes():raise SystemExit('User data disk changed during release validation')
    if source!=sources():raise SystemExit('Sources changed during validation; rerun tests')
    record={'source_sha256':source,'artifacts':hashes(),'toolchain':toolchain,'passed':tests,
            'reproducible':True,'user_data_preserved':True}
    RECORD.write_text(json.dumps(record,indent=2,sort_keys=True)+'\n')
    print('PASS: complete validation, byte-identical rebuild and user data preservation',flush=True)
def package():
    if not RECORD.is_file():raise SystemExit('No validation evidence: run python3 tools/release.py test')
    record=json.loads(RECORD.read_text())
    if record['source_sha256']!=sources() or record['artifacts']!=hashes() or record['toolchain']!=versions():
        raise SystemExit('Sources, artifacts or toolchain differ from validated release; rerun tests')
    destination=ROOT/'build/release';destination.mkdir(exist_ok=True)
    manifest=dict(record,git_commit=output('git','rev-parse','HEAD'),
                  dirty=bool(output('git','status','--porcelain')),
                  scope='QEMU BIOS/UEFI and virtual USB; physical hardware deferred',
                  sizes={name:(ROOT/'build'/name).stat().st_size for name in ARTIFACTS})
    manifest_bytes=(json.dumps(manifest,indent=2,sort_keys=True)+'\n').encode()
    files={name:(ROOT/'build'/name).read_bytes() for name in ARTIFACTS}
    files['manifest.json']=manifest_bytes
    files['README.md']=(ROOT/'docs/RELEASE.md').read_bytes()
    for name in ['PHASE22.md','PHASE23.md','PHASE24.md']:files['docs/'+name]=(ROOT/'docs'/name).read_bytes()
    for name in ['FONT-LICENSE.txt','GNU-EFI-LICENSE.txt']:files['licenses/'+name]=(ROOT/'assets'/name).read_bytes()
    sums=''.join(f'{sha256(data).hexdigest()}  {name}\n' for name,data in sorted(files.items()))
    files['SHA256SUMS']=sums.encode()
    archive=destination/'myos-desktop.zip'
    with zipfile.ZipFile(archive,'w',compression=zipfile.ZIP_DEFLATED,compresslevel=9) as z:
        for name,data in sorted(files.items()):
            item=zipfile.ZipInfo(name,date_time=(2000,1,1,0,0,0));item.compress_type=zipfile.ZIP_DEFLATED
            item.external_attr=0o100644<<16;z.writestr(item,data)
    (destination/'manifest.json').write_bytes(manifest_bytes)
    (destination/'SHA256SUMS').write_text(f'{digest(archive)}  {archive.name}\n')
    print(f'Validated release: {archive}',flush=True)
def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('action',choices=['all','build','test','package'],default='all',nargs='?')
    action=p.parse_args().action
    if action=='build':versions();build()
    elif action=='test':test()
    elif action=='package':package()
    else:test();package()
if __name__=='__main__':main()
