"""Private source-core tooltip retest. Not YouTube/audio acceptance.
Host serves an ordinary verified HTTPS fixture and records the guest display.
Requires an explicit, hash-verified source-built core manifest; normal core untouched.
"""
from pathlib import Path
import os, sys, json, hashlib, subprocess, time, re, shutil
from PIL import Image, ImageChops
from test_system import ROOT, VM
from test_shell import command, type_text

production=Path('/home/xytron404/myos')
workspace=production/'build/phase335-core-workspace'
source=workspace/'src'
manifest_path=Path(os.environ['MYOS_TOOLTIP_CORE_MANIFEST']).resolve()
assert manifest_path.is_relative_to(workspace.resolve())
manifest=json.loads(manifest_path.read_text())
identity=json.loads((production/'docs/wip/phase335/checkpoint7/tooltip-core-build/patch-identity.json').read_text())
sha=lambda path:hashlib.sha256(path.read_bytes()).hexdigest()
assert manifest['build_completed'] is True
assert manifest['source_commit']==identity['source_commit']
assert subprocess.check_output(['git','rev-parse','HEAD'],cwd=source,text=True).strip()==identity['source_commit']
assert sha(source/identity['path'])==identity['modified_sha256']
core_path=Path(manifest['core_path']).resolve()
assert core_path.is_relative_to((source/'out/evergreen-x64_gold').resolve())
assert sha(core_path)==manifest['core_sha256']
assert manifest['source_patch_sha256']==sha(production/'docs/wip/phase335/checkpoint7/tooltip-core-build/tooltip-in-root.patch')
os.environ['MYOS_RENDERER_PROVIDER_PATH']=str(production/'build/media-evaluation/provider-3k2mykzc')
setup=(ROOT/'tests/test_cobalt_native_acceptance.py').read_text().split('bad={s.label:')[0]
old="core=ROOT/'build/engine-evaluation/libcobalt.so'"
assert setup.count(old)==1
setup=setup.replace(old,'core=core_path')
old="assert hashlib.sha256(core.read_bytes()).hexdigest()==audit['library_sha256']"
assert setup.count(old)==1
setup=setup.replace(old,"assert hashlib.sha256(core.read_bytes()).hexdigest()==manifest['core_sha256']")
old="assert provider_directory.is_relative_to((ROOT/'build/renderer-provider-evaluation').resolve())"
assert setup.count(old)==1
setup=setup.replace(old,"assert provider_directory==production/'build/media-evaluation/provider-3k2mykzc'")
exec(compile(setup,'verified-source-tooltip-tls-setup','exec'),globals())
page=b"""<!doctype html><html><head><meta charset="utf-8"><style>body{margin:0;background:#ddeeff}button{position:absolute;left:80px;top:150px;width:400px;height:100px;font:25px sans-serif}</style></head><body><h1>Native tooltip source-core retest</h1><button title="Native tooltip within MyOS">Hover this native HTML title</button><script src="/app.js"></script></body></html>"""
script=b"var reports=[];function report(s){var i=new Image();reports.push(i);i.src='/evidence?kind='+s;}report('ready');document.onmousemove=function(e){report('pointer&x='+e.clientX+'&y='+e.clientY);};"
disk=out/'assets.img';create(disk,specs,runtime=ROOT/'build/platform-runtime.mrt')
for server in servers:
 thread=threading.Thread(target=server.serve_forever,daemon=True);thread.start();threads.append(thread)
sys.path.insert(0,str(ROOT/'docs/wip/native-pmm512/kvm-ata-20261008'))
from qemu_kvm_optin_20261008 import install
install()
from test_cobalt_youtube_observation import native_click
vm=VM('tooltip-fixed',ram=1024,asset_disk=disk,cpu='max',nic=True,audio=out/'audio.wav')
observations=[]
def snapshot(name):
 path=out/(name+'.ppm');vm.command('screendump',{'filename':str(path)})
 with Image.open(path) as original:
  image=original.convert('RGB');image.save(out/(name+'.png'));return image
try:
 vm.wait('myos> ');command(vm,'net up','Network: DHCP configured');command(vm,'mkdir /dev/shm','myos> ')
 for launch in range(2):
  offset=len(vm.log.read_text());record_offset=len(records)
  type_text(vm,'elf cobalt cobalt provider https://cobalt.example.com:'+str(servers[0].server_port)+'/ cobalt.example.com=10.0.2.2\n')
  deadline=time.monotonic()+240;visible=False
  while time.monotonic()<deadline:
   tail=vm.log.read_text()[offset:]
   assert ':FATAL:' not in tail and 'USER SIGNAL' not in tail,tail[-3000:]
   assert not tail.endswith('myos> '),'Engine exited before actual visible DOM'
   if any('kind=ready' in event['path'] for event in records[record_offset:]):
    image=snapshot('launch%d-before-hover'%launch)
    blue=sum(count for count,color in image.getcolors(800*600) or [] if color==(221,238,255))
    if blue>50000:visible=True;break
   time.sleep(1)
  assert visible,'Native DOM callback alone is insufficient: expected rendered page absent'
  launch_observation=dict(launch=launch,visible_background_pixels=blue,hover_cycles=[])
  for cycle in range(2):
   native_click(vm,600,400,None);time.sleep(3)
   before=snapshot('launch%d-cycle%d-before'%(launch,cycle))
   native_click(vm,180,200,None)
   time.sleep(5)
   tail=vm.log.read_text()[offset:]
   assert ':FATAL:' not in tail and 'USER SIGNAL' not in tail and not tail.endswith('myos> '),tail[-3000:]
   hover=snapshot('launch%d-cycle%d-hover'%(launch,cycle))
   native_click(vm,600,400,None);time.sleep(3)
   away=snapshot('launch%d-cycle%d-away'%(launch,cycle))
   # Record real framebuffer differences; inspect the tooltip itself in saved
   # images. Pixel differences or absence of abort do not prove correct drawing.
   launch_observation['hover_cycles'].append(dict(cycle=cycle,hover_change_bbox=ImageChops.difference(before,hover).getbbox(),away_change_bbox=ImageChops.difference(hover,away).getbbox(),drawing_verified=False))
  assert any('kind=pointer&x=180&y=200' in event['path'] for event in records[record_offset:]),'No actual native JS pointer callback'
  vm.key('esc');vm.wait('myos> ')
  tail=vm.log.read_text()[offset:]
  assert ':FATAL:' not in tail and 'USER SIGNAL' not in tail,tail[-3000:]
  resident=re.search(r'ELF resident: free-before=(\d+) free-after=(\d+) shm-before=(\d+) shm-after=(\d+) aliases-before=(\d+) aliases-after=(\d+)',tail)
  assert resident and all(resident[i]==resident[i+1] for i in (1,3,5)),tail[-3000:]
  heap=re.search(r'ELF pair live: pages reclaimed; heap-before=(\d+) heap-after=(\d+)',tail)
  launch_observation['resident_pairs']=resident.groups();launch_observation['heap_pair']=heap.groups() if heap else None
  launch_observation['window_requests']=[line for line in tail.splitlines() if 'window-create' in line]
  observations.append(launch_observation)
  command(vm,'mem','myos> ')
 print('PASS only native DOM/hover survival/recreation/close and physical/SHM/alias cleanup; inspect actual tooltip PNGs before drawing acceptance',flush=True)
finally:
 vm.close();shutil.copyfile(vm.log,out/'serial.log')
 (out/'request-metadata.json').write_text(json.dumps(records,indent=2)+'\n')
 (out/'source-core-manifest.json').write_text(json.dumps(manifest,indent=2)+'\n')
 (out/'tooltip-observations.json').write_text(json.dumps(dict(launches=observations,drawing_verified=False,phase_passed=False,youtube_test=False),indent=2)+'\n')
 for server in servers:server.shutdown();server.server_close()
print('PRIVATE SOURCE TOOLTIP BUNDLE',out,flush=True)
