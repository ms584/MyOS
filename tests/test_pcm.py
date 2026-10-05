"""PRIVATE process-owned PCM real QEMU SB16, not Starboard sink completion."""
from pathlib import Path
import subprocess,sys,tempfile,hashlib
from test_system import ROOT,VM
from test_elf_dynamic import long_command,assert_live_resources
sys.path.insert(0,str(ROOT/'tools'))
from make_asset_disk import create
out=Path(tempfile.mkdtemp(prefix='pcm-',dir=ROOT/'build'));assets=[]
include=subprocess.check_output(['gcc','-print-file-name=include'],text=True).strip()
for name,audio,leak in [('absent',0,0),('pcm',1,0),('cleanup',1,1)]:
 obj=out/(name+'.o');lib=out/(name+'.so')
 subprocess.run(['gcc','-m64','-std=c11','-Os','-fPIC','-ffreestanding','-fno-builtin','-mno-red-zone',
  '-fno-stack-protector','-fno-asynchronous-unwind-tables','-fno-unwind-tables','-nostdinc',
  '-isystem',include,'-Iuser/sdk','-Iinclude','-Wall','-Wextra','-Werror',f'-DEXPECT_AUDIO={audio}',
  f'-DLEAK_AUDIO={leak}','-c','user/elf-pcm.c','-o',str(obj)],check=True)
 subprocess.run(['ld','-shared','--hash-style=both','-z','now','-z','relro','-z','noexecstack',
  '-e','_start','-o',str(lib),str(obj)],check=True);assets.append(name+'='+str(lib))
disk=out/'assets.img';create(disk,assets);disk_hash=hashlib.sha256(disk.read_bytes()).hexdigest()
uefi='--uefi' in sys.argv
options=dict(ram=64,asset_disk=disk,boot_image=ROOT/'build'/('myos-uefi.img' if uefi else 'myos-current.img'))
if uefi:options.update(boot_bus='ide',firmware=(Path('/usr/share/OVMF/OVMF_CODE_4M.fd'),Path('/usr/share/OVMF/OVMF_VARS_4M.fd')))
for hardware in (False,True):
 wave=out/'capture.wav';vm=VM('pcm'+('-audio' if hardware else '-none')+('-uefi' if uefi else ''),audio=wave if hardware else None,**options)
 try:
  vm.wait('myos> ')
  for name in (('cleanup','pcm','cleanup','pcm') if hardware else ('absent','absent')):
   result=long_command(vm,'elf live '+name)
   assert ('native PCM owned IRQ/flush/pointer' if hardware else 'native PCM missing-device fail-closed PASS') in result,result
   assert ('ELF exited with status 92' if hardware else 'ELF exited with status 91') in result,result
   assert_live_resources(result)
 finally:vm.close()
 if hardware:
  import wave as wav,array,statistics
  with wav.open(str(wave),'rb') as f:
   assert f.getsampwidth()==2
   channels=f.getnchannels();rate=f.getframerate()
   samples=array.array('h',f.readframes(f.getnframes()))
  assert any(abs(s)>3000 for s in samples),'no real captured PCM signal'
  mono=samples[::channels]
  # Host resampling can insert values close to zero between native PCM levels.
  # Test amplitude separately, then measure positive zero-crossing periods.
  crossings=[i for i in range(1,len(mono)) if mono[i-1]<0 and mono[i]>0]
  periods=[b-a for a,b in zip(crossings,crossings[1:]) if rate/460<=b-a<=rate/420]
  assert len(periods)>200,'no sustained actual 440Hz PCM output'
  assert 420<rate/statistics.median(periods)<460
 assert hashlib.sha256(disk.read_bytes()).hexdigest()==disk_hash
print('PASS: PRIVATE PCM real SB16/IRQ/pointer/flush/explicit and automatic process exit/relaunch/no-device/exact resources.',out,flush=True)
