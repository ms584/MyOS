"""PRIVATE native Cobalt tooltip diagnostic; HTML fixture, never YouTube acceptance."""
from pathlib import Path
import os,sys,json,time,shutil,re
from test_system import ROOT,VM
from test_shell import command,type_text
from test_elf_dynamic import assert_live_resources
os.environ['MYOS_RENDERER_PROVIDER_PATH']='/home/xytron404/myos/build/media-evaluation/provider-3k2mykzc'
setup=(ROOT/'tests/test_cobalt_native_acceptance.py').read_text().split("bad={s.label:")[0]
setup=setup.replace("assert provider_directory.is_relative_to((ROOT/'build/renderer-provider-evaluation').resolve())", "assert str(provider_directory)=='/home/xytron404/myos/build/media-evaluation/provider-3k2mykzc'")
exec(compile(setup,'sealed-tooltip-tls-setup','exec'),globals())
page=b'''<!doctype html><html><head><meta charset="utf-8"><style>body{background:#ddeeff}button{position:absolute;left:80px;top:150px;width:400px;height:100px;font:25px sans-serif}</style></head><body><h1>Native tooltip reproduction</h1><button title="Native tooltip within MyOS">Hover this native HTML title</button><script src="/app.js"></script></body></html>'''
script=b"var i=new Image();i.src='/evidence?kind=ready';document.onmousemove=function(e){var p=new Image();p.src='/evidence?kind=pointer&x='+e.clientX+'&y='+e.clientY;};"
disk=out/'assets.img';create(disk,specs,runtime=ROOT/'build/platform-runtime.mrt')
for server in servers:
 thread=threading.Thread(target=server.serve_forever,daemon=True);thread.start();threads.append(thread)
sys.path.insert(0,str(ROOT/'docs/wip/native-pmm512/kvm-ata-20261008'))
from qemu_kvm_optin_20261008 import install
install()
from test_cobalt_youtube_observation import native_click
vm=VM('tooltip-repro',ram=1024,asset_disk=disk,cpu='max',nic=True,audio=out/'audio.wav')
try:
 vm.wait('myos> ');command(vm,'net up','Network: DHCP configured');command(vm,'mkdir /dev/shm','myos> ')
 offset=len(vm.log.read_text());type_text(vm,'elf cobalt cobalt provider https://cobalt.example.com:'+str(servers[0].server_port)+'/ cobalt.example.com=10.0.2.2\n')
 deadline=time.monotonic()+210
 while time.monotonic()<deadline:
  if any('kind=ready' in r['path'] for r in records):break
  if vm.log.read_text().endswith('myos> '):raise AssertionError('Engine exited before DOM ready')
  time.sleep(.1)
 else:raise AssertionError('No native DOM ready')
 vm.command('screendump',{'filename':str(out/'before-hover.ppm')})
 print('Actual native JS ready; moving actual PS/2 pointer to title',flush=True)
 native_click(vm,180,200,None)
 deadline=time.monotonic()+15
 while time.monotonic()<deadline and not vm.log.read_text().endswith('myos> '):time.sleep(.1)
 result=vm.log.read_text()[offset:]
 if not result.endswith('myos> '):
  vm.command('screendump',{'filename':str(out/'hover.ppm')});vm.key('esc');vm.wait('myos> ')
  result=vm.log.read_text()[offset:]
 (out/'request-metadata.json').write_text(json.dumps(records,indent=2)+'\n')
 assert any('kind=pointer' in r['path'] for r in records),records
 resident=re.search(r'ELF resident: free-before=(\d+) free-after=(\d+) shm-before=(\d+) shm-after=(\d+) aliases-before=(\d+) aliases-after=(\d+)',result)
 assert resident and all(resident[i]==resident[i+1] for i in (1,3,5)),result
 heap=re.search(r'ELF pair live: pages reclaimed; heap-before=(\d+) heap-after=(\d+)',result)
 print('Physical/SHM/alias exact; heap values separately',heap.groups() if heap else 'not captured',flush=True)
 if ':FATAL:' in result and 'USER SIGNAL' in result:
  print('REPRO native tooltip FATAL/SIGABRT; no repair or acceptance',flush=True)
 else:print('NOT REPRODUCED; no repair or acceptance',flush=True)
 print('NUMERIC WINDOW DIAGNOSTIC',*[l for l in result.splitlines() if 'window-create' in l],sep='\n',flush=True)
finally:
 vm.close();shutil.copyfile(vm.log,out/'serial.log')
 for server in servers:server.shutdown();server.server_close()
print('PRIVATE TOOLTIP BUNDLE',out,flush=True)
