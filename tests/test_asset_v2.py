"""PRIVATE native asset resources; genuine ELF/runtime remains in MyOS."""
from pathlib import Path
import hashlib
import subprocess
import tempfile
import sys
from test_system import ROOT,VM
from test_elf_dynamic import long_command,assert_live_resources
sys.path.insert(0,str(ROOT/'tools'))
from make_asset_disk import create
out=Path(tempfile.mkdtemp(prefix='asset-v2-',dir=ROOT/'build'))
obj=out/'fixture.o';lib=out/'fixture.so'
include=subprocess.check_output(['gcc','-print-file-name=include'],text=True).strip()
subprocess.run(['gcc','-m64','-std=c11','-Os','-fPIC','-ffreestanding','-fno-builtin',
 '-mno-red-zone','-fno-stack-protector','-fno-asynchronous-unwind-tables','-fno-unwind-tables',
 '-nostdinc','-isystem',include,'-Iuser/sdk','-Iinclude','-Wall','-Wextra','-Werror',
 '-c','user/elf-asset-v2.c','-o',str(obj)],check=True)
subprocess.run(['ld','-shared','--hash-style=both','-z','now','-z','relro','-z','noexecstack',
 '-e','_start','-o',str(lib),str(obj)],check=True)
font=out/'font.bin';font.write_bytes(bytes((i*13+7)&255 for i in range(65537)))
small=out/'small.bin';small.write_bytes(b'controlled-resource')
disk=out/'assets.img';create(disk,['hierarchy='+str(lib),'root.txt='+str(small),
 'content/fonts/font.bin='+str(font),'content/icu/data.bin='+str(small),'content/cert.pem='+str(small),
 'content/fonts/resource-name-that-exceeds-legacy-directory-metadata-limit-for-browser-fonts.bin='+str(small)])
uefi='--uefi' in sys.argv
options=dict(ram=64,boot_image=ROOT/'build'/('myos-uefi.img' if uefi else 'myos-current.img'),asset_disk=disk)
if uefi:options.update(boot_bus='ide',firmware=(Path('/usr/share/OVMF/OVMF_CODE_4M.fd'),Path('/usr/share/OVMF/OVMF_VARS_4M.fd')))
before=hashlib.sha256(disk.read_bytes()).hexdigest()
vm=VM('asset-v2'+('-uefi' if uefi else ''),**options)
try:
 vm.wait('myos> ')
 for _ in range(4):
  result=long_command(vm,'elf live hierarchy')
  assert 'native v2 hierarchy/hidden runtime/long names/large I/O/readonly/pointer ABI PASS' in result,result
  assert 'ELF exited with status 90' in result,result
  assert_live_resources(result)
finally:vm.close()
assert before==hashlib.sha256(disk.read_bytes()).hexdigest()
print('PASS: PRIVATE native v2 hidden runtime/resources/large I/O/readonly/relaunch/exact resources.',out,flush=True)
legacy=out/'legacy-v1.img';create(legacy,['root.txt='+str(small)],runtime=None,version=1)
options['asset_disk']=legacy
legacy_hash=hashlib.sha256(legacy.read_bytes()).hexdigest()
vm=VM('asset-v1-compat'+('-uefi' if uefi else ''),**options)
try:
 vm.wait('myos> ')
 result=long_command(vm,'cat /assets/root.txt');assert 'controlled-resource' in result,result
 result=long_command(vm,'ls /assets');assert 'root.txt' in result,result
finally:vm.close()
assert hashlib.sha256(legacy.read_bytes()).hexdigest()==legacy_hash
print('PASS: PRIVATE native v1 readonly parser/ATA/shell compatibility; not external-runtime execution.',out,flush=True)
