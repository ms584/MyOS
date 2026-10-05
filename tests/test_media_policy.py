"""PRIVATE native pinned media policy. No decoder/player or Cobalt acceptance."""
from pathlib import Path
import hashlib,json,subprocess,sys,tempfile
from test_system import ROOT,VM
from test_elf_dynamic import long_command,assert_live_resources
sys.path.insert(0,str(ROOT/'tools'))
from make_asset_disk import create
pins=json.loads((ROOT/'docs/PHASE331_ABI_SOURCES.json').read_text())['headers']
for name in ('media.h','drm.h'):
 expected=next(item['sha256'] for item in pins if item['path']=='starboard/'+name)
 assert hashlib.sha256((ROOT/'user/starboard-media/include/starboard'/name).read_bytes()).hexdigest()==expected
if '--headers-only' in sys.argv:
 print('PASS: media/drm pinned SHA before compilation',flush=True);raise SystemExit(0)
out=Path(tempfile.mkdtemp(prefix='media-policy-',dir=ROOT/'build'))
intrinsics=subprocess.check_output(['gcc','-print-file-name=include'],text=True).strip()
obj=out/'fixture.o';lib=out/'fixture.so'
subprocess.run(['gcc','-m64','-std=c11','-Os','-fPIC','-ffreestanding','-fno-builtin','-mno-red-zone',
 '-fno-stack-protector','-fno-asynchronous-unwind-tables','-fno-unwind-tables','-nostdinc',
 '-isystem',intrinsics,'-Iuser/sdk','-Iuser/starboard-media/include','-Iuser/starboard-system/include',
 '-Wall','-Wextra','-Werror','-c','user/elf-media-policy.c','-o',str(obj)],check=True)
subprocess.run(['ld','-shared','--hash-style=both','-z','now','-z','relro','-z','noexecstack','-e','_start',
 '-o',str(lib),str(obj)],check=True)
disk=out/'assets.img';create(disk,['policy='+str(lib)]);before=hashlib.sha256(disk.read_bytes()).hexdigest()
uefi='--uefi' in sys.argv
for ram in (64,256):
 options=dict(ram=ram,asset_disk=disk,boot_image=ROOT/'build'/('myos-uefi.img' if uefi else 'myos-current.img'))
 if uefi:options.update(boot_bus='ide',firmware=(Path('/usr/share/OVMF/OVMF_CODE_4M.fd'),Path('/usr/share/OVMF/OVMF_VARS_4M.fd')))
 vm=VM('media-policy-'+str(ram)+('-uefi' if uefi else ''),**options)
 try:
  vm.wait('myos> ')
  for _ in range(4):
   result=long_command(vm,'elf live policy')
   assert 'native pinned media policy unsupported codecs/DRM/changeType/direct malloc/170s/RAM-derived budgets/threads PASS' in result,result
   assert 'ELF exited with status 96' in result,result
   assert_live_resources(result)
 finally:vm.close()
 assert hashlib.sha256(disk.read_bytes()).hexdigest()==before
print('PASS: PRIVATE native pinned media policy, actual memory budget at64/256MiB, concurrent workers/relaunch/resources; NO codecs/player/Cobalt.',out,flush=True)
