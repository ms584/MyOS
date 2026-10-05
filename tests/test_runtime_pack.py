"""MRT1 exact resident bytes, fail-closed corruption, RX and reclamation."""
import argparse
import hashlib
import json
import re
import random
import struct
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
from test_system import ROOT, VM
from test_elf_dynamic import long_command, assert_live_resources
sys.path.insert(0, str(ROOT/'tools'))
from pack_runtime import pack
from make_asset_disk import create

p=argparse.ArgumentParser(description=__doc__)
p.add_argument('--uefi',action='store_true')
p.add_argument('--host-only',action='store_true')
args=p.parse_args()
base=ROOT/'build/runtime-pack-evaluation';base.mkdir(parents=True,exist_ok=True)
out=Path(tempfile.mkdtemp(prefix='pack-',dir=base))
raw=(ROOT/'build/platform-runtime.bin').read_bytes()
packed=(ROOT/'build/platform-runtime.mrt').read_bytes()
assert packed==pack(raw)
check=out/'check'
subprocess.run(['gcc','-std=c11','-O1','-g','-fsanitize=address,undefined',
 '-fno-omit-frame-pointer','-Wall','-Wextra','-Werror','-Wno-misleading-indentation',
 '-Iinclude','kernel/runtime-pack.c','tests/runtime-pack-check.c','-o',str(check)],check=True)
subprocess.run([str(check),'build/platform-runtime.mrt','build/platform-runtime.bin'],check=True)
random_bytes=random.Random(33)
for length in (1,2,3,127,128,129,130,131,65535,65536):
 for kind in ('literal','overlap'):
  data=bytes(random_bytes.randrange(256) for _ in range(length)) if kind=='literal' else bytes(length)
  original=out/f'{kind}-{length}.bin';encoded=out/f'{kind}-{length}.mrt'
  original.write_bytes(data);encoded.write_bytes(pack(data))
  subprocess.run([str(check),str(encoded),str(original),'sample'],check=True)
for data in (b'',bytes(65537)):
 try:pack(data)
 except ValueError:pass
 else:raise AssertionError('invalid raw size accepted')
if args.host_only:
 print('PASS: host runtime packing; NOT native.',out,flush=True)
 raise SystemExit(0)
value=14695981039346656037
for byte in raw:value=((value^byte)*1099511628211)&((1<<64)-1)
assets=[]
for name,defines in [('exact',[]),('rx',['-DWRITE_RX=1'])]:
 obj=out/(name+'.o');lib=out/(name+'.so')
 subprocess.run(['gcc','-m64','-std=c11','-Os','-fPIC','-ffreestanding',
  '-fno-builtin','-mno-red-zone','-fno-stack-protector','-fno-asynchronous-unwind-tables',
  '-fno-unwind-tables','-Wall','-Wextra','-Werror',f'-DRAW_SIZE={len(raw)}u',
  f'-DRAW_HASH={value}ull',*defines,'-c','user/elf-runtime-pack.c','-o',str(obj)],check=True)
 subprocess.run(['ld','-shared','--hash-style=both','-z','now','-z','relro',
  '-z','noexecstack','-e','_start','-o',str(lib),str(obj)],check=True)
 assets.append(name+'='+str(lib))
disk=out/'assets.img';create(disk,assets)
boot=ROOT/'build'/('myos-uefi.img' if args.uefi else 'myos-current.img')
before=hashlib.sha256(boot.read_bytes()).hexdigest()
frozen=out/'boot.img';shutil.copyfile(boot,frozen)
options=dict(ram=64,boot_image=frozen,asset_disk=disk)
if args.uefi:options.update(boot_bus='ide',firmware=(Path('/usr/share/OVMF/OVMF_CODE_4M.fd'),Path('/usr/share/OVMF/OVMF_VARS_4M.fd')))
vm=VM('runtime-pack'+('-uefi' if args.uefi else ''),**options)
try:
 vm.wait('myos> ')
 for _ in range(3):
  result=long_command(vm,'elf live exact')
  assert 'mapped runtime exact bytes PASS' in result and 'ELF exited with status 88' in result,result
  assert_live_resources(result)
  result=long_command(vm,'elf live rx')
  assert 'mapped runtime exact bytes PASS' in result and 'USER FAULT' in result,result
  assert 'vector=14 error=0x0000000000000007' in result and 'address=0x000000803f000000' in result.lower(),result
  assert_live_resources(result)
finally:vm.close();shutil.copyfile(vm.log,out/'serial.log')
# Embedded BIOS corruption and external-runtime corruption use separate private
# objects. External mode is checked on both boot profiles; never mutate the
# shared normal image or its resource disk.
manifest=ROOT/'build/platform-runtime.manifest.json'
external=manifest.exists() and json.loads(manifest.read_text())['external']
if external or not args.uefi:
 if external:
  image=bytearray(disk.read_bytes())
  assert image[:13]==b'MYOS-ASSET-v2'
  offset,length=struct.unpack_from('<QQ',image,32)
  assert length==len(packed) and image[offset:offset+length]==packed
  image[offset+12]^=1
  damaged=out/'bad-crc-assets.img';damaged.write_bytes(image)
  damaged_options=dict(options,asset_disk=damaged)
 else:
  symbols=subprocess.check_output(['nm','-n','build/kernel.elf'],text=True).splitlines()
  start=int(next(line.split()[0] for line in symbols if line.endswith(' platform_runtime_start')),16)
  offset=17*512+start-0x10000
  image=bytearray(frozen.read_bytes());assert image[offset:offset+len(packed)]==packed
  image[offset+12]^=1
  damaged=out/'bad-crc.img';damaged.write_bytes(image)
  damaged_options=dict(options,boot_image=damaged)
 damaged_hash=hashlib.sha256(damaged.read_bytes()).hexdigest()
 vm=VM('runtime-pack-corrupt'+('-uefi' if args.uefi else ''),**damaged_options)
 try:
  vm.wait('myos> ')
  def pages():
   result=long_command(vm,'mem')
   match=re.search(r'Free physical pages: (\d+)',result)
   assert match,result
   return int(match[1])
  baseline=pages()
  for _ in range(3):
   result=long_command(vm,'elf live exact')
   assert 'out of memory' in result.lower(),result
   if external:assert 'Runtime asset: unavailable or invalid build identity' in result,result
   assert 'mapped runtime exact bytes PASS' not in result and 'ELF live: started' not in result,result
   assert 'ELF staged image released; not executed' in result and 'PANIC' not in result,result
   assert pages()==baseline,'corrupt-runtime page leak'
 finally:vm.close();shutil.copyfile(vm.log,out/'corrupt-serial.log')
 assert hashlib.sha256(damaged.read_bytes()).hexdigest()==damaged_hash
assert before==hashlib.sha256(boot.read_bytes()).hexdigest()
print('PASS: runtime exact mapped bytes/RX faults/relaunch/resources',vars(args),out,flush=True)
