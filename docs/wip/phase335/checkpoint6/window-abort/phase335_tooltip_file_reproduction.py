"""PRIVATE native HTML-title diagnostic from an ordinary Cobalt content asset; never online/media acceptance."""
from pathlib import Path
import json,hashlib,sys,tempfile,time,shutil
from test_system import ROOT,VM
from test_shell import type_text,command
from test_elf_dynamic import assert_live_resources
sys.path.insert(0,str(ROOT/'tools'))
from make_asset_disk import create
out=Path(tempfile.mkdtemp(prefix='tooltip-file-',dir=ROOT/'build'))
core=ROOT/'build/engine-evaluation/libcobalt.so';provider=Path('/home/xytron404/myos/build/media-evaluation/provider-3k2mykzc/provider.so')
audit=json.loads((ROOT/'docs/PHASE33_COBALT_AUDIT.json').read_text())
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
assert sha(core)==audit['library_sha256']
assert sha(provider)==json.loads((provider.parent/'report.json').read_text())['provider_sha256']
specs=['cobalt='+str(core),'provider='+str(provider)]
resources=Path(sys.argv[1]);r=json.loads((resources/'report.json').read_text());assert r['package_sha256']==audit['package_sha256']
for item in r['resources']:
 p=resources/'content'/item['name'];assert sha(p)==item['sha256'];specs.append(item['name']+'='+str(p))
fonts=Path(sys.argv[2])
for item in json.loads((fonts/'report.json').read_text())['resources']:
 p=fonts/item['name'];assert sha(p)==item['sha256'];specs.append('systemfonts/'+item['name']+'='+str(p))
page=out/'tooltip.html';page.write_text('<!doctype html><meta charset="utf-8"><style>body{background:#ddeeff}button{position:absolute;left:80px;top:150px;width:400px;height:100px;font:25px sans-serif}</style><h1>Native tooltip reproduction</h1><button title="Native tooltip within MyOS">Hover this native HTML title</button>')
specs.append('tooltip.html='+str(page));disk=out/'assets.img';create(disk,specs,runtime=ROOT/'build/platform-runtime.mrt')
sys.path.insert(0,str(ROOT/'docs/wip/native-pmm512/kvm-ata-20261008'))
from qemu_kvm_optin_20261008 import install
install()
from test_cobalt_youtube_observation import native_click
vm=VM('tooltip-file',ram=1024,asset_disk=disk,cpu='max',audio=out/'audio.wav')
try:
 vm.wait('myos> ');command(vm,'mkdir /dev/shm','myos> ');offset=len(vm.log.read_text());type_text(vm,'elf cobalt cobalt provider file:///cobalt/tooltip.html\n')
 deadline=time.monotonic()+150
 while time.monotonic()<deadline:
  text=vm.log.read_text()[offset:]
  if 'DIAG window-create' in text:break
  if text.endswith('myos> '):raise AssertionError('Engine exited before window')
  time.sleep(.1)
 else:raise AssertionError('No native engine window')
 time.sleep(20);vm.command('screendump',{'filename':str(out/'before-hover.ppm')})
 print('Native compositor frame saved; moving actual pointer',flush=True);native_click(vm,180,200,None)
 deadline=time.monotonic()+15
 while time.monotonic()<deadline and not vm.log.read_text().endswith('myos> '):time.sleep(.1)
 text=vm.log.read_text()[offset:]
 if not text.endswith('myos> '):
  vm.command('screendump',{'filename':str(out/'hover.ppm')});vm.key('esc');vm.wait('myos> ');text=vm.log.read_text()[offset:]
 assert_live_resources(text)
 print('REPRO native tooltip FATAL/SIGABRT' if ':FATAL:' in text and 'USER SIGNAL' in text else 'NOT REPRODUCED',flush=True)
 print(*[l for l in text.splitlines() if 'window-create' in l],sep='\n',flush=True)
finally:vm.close();shutil.copyfile(vm.log,out/'serial.log')
print('PRIVATE FILE TOOLTIP BUNDLE',out,'NOT REPAIRED/NOT ACCEPTED',flush=True)
