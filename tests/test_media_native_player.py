"""Native asset player checkpoint. Does NOT establish Cobalt/HTTPS acceptance."""
from pathlib import Path
import hashlib,json,subprocess,sys,tempfile,shutil,re,time,wave
from test_system import ROOT,VM
from test_elf_dynamic import long_command,assert_live_resources
sys.path.insert(0,str(ROOT/'tools'))
from make_asset_disk import create
from test_shell import type_text
base=ROOT/'build/media-evaluation'
provider=Path(sys.argv[sys.argv.index('--provider')+1]).resolve()
report=json.loads((provider/'report.json').read_text())
digest=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
assert digest(provider/'provider.so')==report['provider_sha256']
out=Path(tempfile.mkdtemp(prefix='native-player-',dir=base))
cc=base/'target-cc';objects=[]
# EGL header copied only into test include directory, pinned source contract.
headers=out/'include/starboard';headers.mkdir(parents=True)
(headers/'egl.h').write_bytes((ROOT/'build/engine-evaluation/contracts/egl.h').read_bytes())
for name in ('media-webm','elf-media-player'):
 obj=out/(name+'.o');objects.append(obj)
 subprocess.run([str(cc),'-std=c11','-O2','-Wall','-Wextra','-Werror',
 '-Wno-misleading-indentation','-I'+str(out/'include'),'-I'+str(provider/'include'),
 '-Iuser','-Iinclude','-Iuser/starboard-media/include','-Iuser/starboard-system/include',
 '-c','user/'+name+'.c','-o',str(obj)],check=True)
archives=[]
support=json.loads((ROOT/'build/renderer-provider-evaluation/provider-84sokh6g/report.json').read_text())
for item in support['inputs']:
 p=Path(item['path'])
 if p.name in ('libmyos-format.a','libmyos-math.a'):
  assert digest(p)==item['sha256'];archives.append(str(p))
library=out/'player.so'
subprocess.run(['ld','-shared','--hash-style=both','-z','now','-z','relro','-z','noexecstack',
 '-e','_start','-o',str(library),*map(str,objects),'--start-group',*archives,'--end-group'],check=True)
dynamic=subprocess.check_output(['readelf','-dW',str(library)],text=True)
assert '(NEEDED)' not in dynamic and '(TEXTREL)' not in dynamic
undefined=subprocess.check_output(['nm','-u',str(library)],text=True)
(out/'undefined.txt').write_text(undefined)
allow=set(re.findall(r'\bX\((\w+),',(ROOT/'build/platform-runtime-exports.h').read_text()))
allow.update(re.findall(r'X\((\w+)\)',(ROOT/'include/media-provider-abi.h').read_text()))
allow.update(('SbGetEglInterface','SbGetGlesInterface'))
strong={line.split()[-1] for line in undefined.splitlines() if line.split()[0]=='U'}
assert strong<=allow,sorted(strong-allow)
fixture=ROOT/'assets/tests/phase333/standard-160x90-40s.webm'
assert digest(fixture)==json.loads((ROOT/'assets/tests/phase333/standard-fixture.json').read_text())['sha256']
runtime_source=Path(sys.argv[sys.argv.index('--runtime')+1]).resolve() if '--runtime' in sys.argv else ROOT/'build/platform-runtime.mrt'
runtime=out/'platform-runtime.mrt';shutil.copyfile(runtime_source,runtime)
disk=out/'assets.img';create(disk,['player='+str(library),'provider='+str(provider/'provider.so'),'media='+str(fixture)],runtime=runtime)
boot=Path(sys.argv[sys.argv.index('--boot-image')+1]).resolve() if '--boot-image' in sys.argv else ROOT/'build/myos-current.img';before=digest(boot);frozen=out/'boot.img';shutil.copyfile(boot,frozen)
if '--kvm' in sys.argv:
 sys.path.insert(0,str(ROOT/'docs/wip/native-pmm512/kvm-ata-20261008'))
 from qemu_kvm_optin_20261008 import install
 install()
vm=VM('media-native-player',ram=512,boot_image=frozen,asset_disk=disk,audio=out/'audio.wav')
output='';captures=[]
try:
 vm.wait('myos> ')
 # Shell injection without blocking read allows same-session display capture.
 offset=len(vm.log.read_text());type_text(vm,'elf pair-live player provider\n')
 deadline=time.monotonic()+150;next_capture=time.monotonic()+5
 while time.monotonic()<deadline:
  output=vm.log.read_text()[offset:]
  if 'ELF exited with status' in output or 'ELF pair live failed' in output or 'USER FAULT' in output or output.endswith('myos> '):break
  if time.monotonic()>=next_capture:
   image=out/('frame-%03d.ppm'%len(captures));vm.command('screendump',{'filename':str(image)})
   captures.append({'path':str(image),'host_monotonic':time.monotonic(),'sha256':digest(image)})
   next_capture=time.monotonic()+2
  time.sleep(.1)
 print(output,flush=True)
finally:
 vm.close();shutil.copyfile(vm.log,out/'serial.log');assert digest(boot)==before
(out/'evidence.json').write_text(json.dumps({'native_asset_checkpoint':True,'cobalt_media':False,'passed':False,
 'provider_sha256':digest(provider/'provider.so'),'fixture_sha256':digest(fixture),'core_sha256':digest(library),
 'boot_sha256':before,'captures':captures,'output':output},indent=2)+'\n')
assert 'native standard player asset/control/EOS checkpoint PASS' in output,output
assert 'ELF exited with status 97' in output,output
match=re.search(r'ELF pair live: pages reclaimed; heap-before=(\d+) heap-after=(\d+)',output)
assert match and match[1]==match[2],output
from array import array
import math
with wave.open(str(out/'audio.wav'),'rb') as w:
 assert w.getsampwidth()==2
 samples=array('h',w.readframes(w.getnframes()))
 mono=samples[::w.getnchannels()]
 windows=[]
 for i in range(0,len(mono)-w.getframerate()+1,w.getframerate()):
  segment=mono[i:i+w.getframerate()]
  rms=math.sqrt(sum(x*x for x in segment)/len(segment))
  crossings=sum(segment[j-1]<0<=segment[j] for j in range(1,len(segment)))
  windows.append(dict(second=i/w.getframerate(),rms=rms,positive_crossings=crossings))
 audible=[x for x in windows if x['rms']>1000 and 350<=x['positive_crossings']<=950]
 assert len(audible)>=30,('too little actual captured tone',windows)
 evidence_audio=windows
 assert w.getnframes()>30*w.getframerate()
 evidence=json.loads((out/'evidence.json').read_text());evidence.update(passed=True,audio_seconds=w.getnframes()/w.getframerate(),
 audio_channels=w.getnchannels(),audio_rate=w.getframerate(),audio_windows=evidence_audio,audible_seconds=len(audible))
from PIL import Image
visual=[]
for capture in captures:
 p=Path(capture['path']);png=p.with_suffix('.png')
 picture=Image.open(p).convert('RGB');picture.save(png)
 # Count changed guest pixels in the actual displayed video viewport.
 crop=picture.crop((80,340,400,520))
 visual.append(dict(path=png.name,host_monotonic=capture['host_monotonic'],
  pixels_sha256=hashlib.sha256(crop.tobytes()).hexdigest()))
assert len({x['pixels_sha256'] for x in visual})>=12,visual
evidence['visual']=visual
(out/'evidence.json').write_text(json.dumps(evidence,indent=2)+'\n')
print('PASS native standard asset player checkpoint; NOT Cobalt/HTTPS acceptance',out,flush=True)
