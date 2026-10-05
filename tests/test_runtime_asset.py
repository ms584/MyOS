"""PRIVATE external-runtime build identity, RX, missing/corruption recovery."""
from pathlib import Path
import hashlib
import struct
import subprocess
import sys
import tempfile
import re
import zlib
from test_system import ROOT,VM
from test_elf_dynamic import long_command,assert_live_resources
sys.path.insert(0,str(ROOT/'tools'))
from make_asset_disk import create

out=Path(tempfile.mkdtemp(prefix='runtime-asset-',dir=ROOT/'build'))
raw=(ROOT/'build/platform-runtime.bin').read_bytes()
packed=(ROOT/'build/platform-runtime.mrt').read_bytes()
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

# Produce a valid SAME-LENGTH MRT1 carrying different raw bytes and a correct
# CRC. This must fail the compiled SHA256 identity, not merely the CRC decoder.
changed=bytearray(packed)
assert changed[16]<128
changed[17]^=1
decoded=bytearray();index=16
while index<len(changed):
 token=changed[index];index+=1
 if token<128:
  count=token+1;decoded.extend(changed[index:index+count]);index+=count
 else:
  count=(token&127)+3;distance=int.from_bytes(changed[index:index+2],'little');index+=2
  for _ in range(count):decoded.append(decoded[-distance])
assert len(decoded)==len(raw) and decoded!=raw
struct.pack_into('<I',changed,12,zlib.crc32(decoded))
foreign=out/'foreign.mrt';foreign.write_bytes(changed)
truncated=out/'truncated.mrt';truncated.write_bytes(packed[:-1])
badcrc=out/'badcrc.mrt';damaged=bytearray(packed);damaged[12]^=1;badcrc.write_bytes(damaged)
profiles=[('good',ROOT/'build/platform-runtime.mrt'),('missing',None),
          ('wrong-length',truncated),('wrong-build-valid-crc',foreign),('bad-crc',badcrc)]
uefi='--uefi' in sys.argv
boot=ROOT/'build'/('myos-uefi.img' if uefi else 'myos-current.img')
boot_hash=hashlib.sha256(boot.read_bytes()).hexdigest()
for name,runtime in profiles:
 disk=out/(name+'.img')
 create(disk,assets,runtime=runtime,version=2)
 disk_hash=hashlib.sha256(disk.read_bytes()).hexdigest()
 options=dict(ram=64,boot_image=boot,asset_disk=disk)
 if uefi:options.update(boot_bus='ide',firmware=(Path('/usr/share/OVMF/OVMF_CODE_4M.fd'),Path('/usr/share/OVMF/OVMF_VARS_4M.fd')))
 vm=VM('runtime-asset-'+name+('-uefi' if uefi else ''),**options)
 try:
  vm.wait('myos> ')
  def pages():
   result=long_command(vm,'mem');match=re.search(r'Free physical pages: (\d+)',result)
   assert match,result
   return int(match[1])
  baseline=pages()
  for _ in range(3):
   result=long_command(vm,'elf live exact')
   if name=='good':
    assert 'mapped runtime exact bytes PASS' in result and 'ELF exited with status 88' in result,result
    assert_live_resources(result)
    result=long_command(vm,'elf live rx')
    assert 'vector=14 error=0x0000000000000007' in result,result
    assert_live_resources(result)
   else:
    assert 'Runtime asset: unavailable or invalid build identity' in result,result
    assert 'mapped runtime exact bytes PASS' not in result and 'ELF live: started' not in result,result
    assert 'ELF staged image released; not executed' in result and 'PANIC' not in result,result
   assert pages()==baseline,(name,'page leak')
 finally:vm.close()
 assert hashlib.sha256(disk.read_bytes()).hexdigest()==disk_hash
 print('PASS: PRIVATE external runtime',name,'UEFI' if uefi else 'BIOS',flush=True)
assert hashlib.sha256(boot.read_bytes()).hexdigest()==boot_hash
print('PASS: PRIVATE exact mapped bytes/RX/build identity/missing/length/CRC/readonly/relaunch.',out,flush=True)
