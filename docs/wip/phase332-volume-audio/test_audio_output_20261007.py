"""Native device-derived audio metadata; does NOT prove decoding/playback."""
from pathlib import Path
import hashlib,json,subprocess,sys,tempfile
from test_system import ROOT,VM
from test_elf_dynamic import long_command,assert_live_resources
sys.path.insert(0,str(ROOT/'tools'))
from make_asset_disk import create
pin=next(item['sha256'] for item in json.loads((ROOT/'docs/PHASE331_ABI_SOURCES.json').read_text())['headers'] if item['path']=='starboard/media.h')
assert hashlib.sha256((ROOT/'user/starboard-media/include/starboard/media.h').read_bytes()).hexdigest()==pin
out=Path(tempfile.mkdtemp(prefix='audio-output-',dir=ROOT/'build'))
intrinsics=subprocess.check_output(['gcc','-print-file-name=include'],text=True).strip()
assets=[]
for device in (0,1):
 obj=out/f'output{device}.o';lib=obj.with_suffix('.so')
 subprocess.run(['gcc','-m64','-std=c11','-Os','-fPIC','-ffreestanding','-fno-builtin','-mno-red-zone','-fno-stack-protector','-fno-asynchronous-unwind-tables','-fno-unwind-tables','-nostdinc','-isystem',intrinsics,'-Iuser/sdk','-Iuser/starboard-media/include','-Iuser/starboard-system/include','-Wall','-Wextra','-Werror','-DDEVICE='+str(device),'-c','user/elf-audio-output.c','-o',str(obj)],check=True)
 subprocess.run(['ld','-shared','--hash-style=both','-z','now','-z','relro','-z','noexecstack','-e','_start','-o',str(lib),str(obj)],check=True)
 assets.append('output'+str(device)+'='+str(lib))
disk=out/'assets.img';create(disk,assets);before=hashlib.sha256(disk.read_bytes()).hexdigest()
uefi='--uefi' in sys.argv
for device in (0,1):
 options=dict(ram=64,asset_disk=disk,boot_image=ROOT/'build'/('myos-uefi.img' if uefi else 'myos-current.img'),cpu='max')
 if device:options['audio']=out/'device.wav'
 if uefi:options.update(boot_bus='ide',firmware=(Path('/usr/share/OVMF/OVMF_CODE_4M.fd'),Path('/usr/share/OVMF/OVMF_VARS_4M.fd')))
 vm=VM('audio-output-'+str(device)+('-uefi' if uefi else ''),**options)
 try:
  vm.wait('myos> ')
  for _ in range(4):
   result=long_command(vm,'elf live output'+str(device),timeout=45)
   assert 'native pinned audio-output actual SB16/absent device/config/guards/concurrency PASS' in result,result
   assert 'ELF exited with status 97' in result,result
   assert_live_resources(result)
 finally:vm.close()
assert hashlib.sha256(disk.read_bytes()).hexdigest()==before
print('PASS native pinned actual/no-device audio outputs, real mono PCM configuration, invalid output unchanged, guards, workers/relaunch/exact resources; NOT stereo/codec/player',out,flush=True)
