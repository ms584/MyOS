"""Native asset player checkpoint. Does NOT establish Cobalt/HTTPS acceptance."""
from pathlib import Path
import hashlib,json,subprocess,sys,tempfile,shutil,re,time,wave
from test_system import ROOT,VM
from test_elf_dynamic import long_command,assert_live_resources
sys.path.insert(0,str(ROOT/'tools'))
from make_asset_disk import create
from test_shell import type_text
base=ROOT/'build/media-evaluation'
vp9='--vp9' in sys.argv
underlay='--underlay' in sys.argv
video_gap='--video-gap' in sys.argv
late_bounds='--late-bounds' in sys.argv
assert not late_bounds or (vp9 and underlay), 'late-bounds requires native VP9 underlay'
assert not video_gap or (vp9 and underlay), 'video-gap requires native VP9 underlay'
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
 *(['-DMYOS_TEST_VP9=1'] if vp9 else []),*(['-DMYOS_TEST_UNDERLAY=1'] if underlay else []),*(['-DMYOS_TEST_VIDEO_GAP=1'] if video_gap else []),*(['-DMYOS_TEST_LATE_BOUNDS=1'] if late_bounds else []),'-c','user/'+name+'.c','-o',str(obj)],check=True)
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
fixture=base/'standard-vp9-160x90-40s.webm' if vp9 else ROOT/'assets/tests/phase333/standard-160x90-40s.webm'
fixture_metadata=base/'standard-fixture-vp9.json' if vp9 else ROOT/'assets/tests/phase333/standard-fixture.json'
assert digest(fixture)==json.loads(fixture_metadata.read_text())['sha256']
if vp9:assert report['vp9_candidate']
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
(out/'evidence.json').write_text(json.dumps({'output_mode':'underlay' if underlay else 'texture','video_codec':'VP9' if vp9 else 'VP8','native_asset_checkpoint':True,'cobalt_media':False,'passed':False,
 'provider_sha256':digest(provider/'provider.so'),'fixture_sha256':digest(fixture),'core_sha256':digest(library),
 'boot_sha256':before,'captures':captures,'output':output},indent=2)+'\n')
assert 'native standard player asset/control/EOS checkpoint PASS' in output,output
assert 'ELF exited with status 97' in output,output
if underlay:
 metrics=[dict((k,int(v)) for k,v in re.findall(r'([\w-]+)=(\d+)',line))
          for line in output.splitlines() if line.startswith('MEDIA native metrics ')]
 assert len(metrics)==2 and metrics[0]['sync-samples']>=400 and metrics[0]['max-sync-us']<=100000,metrics

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

if video_gap:
 assert 'native VIDEO_GAP held clock PASS; feeder restored' in output, 'Video-only gap did not hold actual audio clock'
 assert 'MEDIA rebuffer pause ' in output and 'MEDIA rebuffer resume ' in output, 'Native rebuffer transition not observed'
 (out/'video-gap-result.json').write_text(json.dumps(dict(native_video_only_gap=True,audio_hardware_clock_held=True,feed_restored=True,phase_passed=False),indent=2)+chr(10))
 print('PASS native video-only gap: actual audio held, feeding restored, controls/EOS/resources passed; not phase acceptance')

if late_bounds:
 assert 'native LATE_BOUNDS clock held until visible bounds PASS' in output
 first=re.findall(r'MEDIA first-video ticket=(\d+) pts=(\d+) clock=(\d+) consumed=(\d+)',output)
 assert first and all(int(row[3])==0 for row in first), 'Audio ran before first video submission'
 (out/'late-bounds-result.json').write_text(json.dumps(dict(native_late_bounds=True,actual_clock_held_until_first_video=True,first_video_rows=first,phase_passed=False),indent=2)+chr(10))
 print('PASS native late SetBounds: actual audio held until first video; controls/EOS/resources passed')
