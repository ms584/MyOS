"""Genuine guest HTTP/DOM/timer/input observation; NOT HTTPS/full acceptance."""
from pathlib import Path
import hashlib
import http.server
import json
import subprocess
import sys
import tempfile
import threading
import time
from test_system import ROOT,VM
from test_shell import command,type_text
from test_elf_dynamic import long_command
sys.path.insert(0,str(ROOT/'tools'))
from make_asset_disk import create
core=ROOT/'build/engine-evaluation/libcobalt.so'
provider=ROOT/'build/renderer-provider-evaluation/provider-84sokh6g/provider.so'
assert hashlib.sha256(core.read_bytes()).hexdigest()==json.loads((ROOT/'docs/PHASE33_COBALT_AUDIT.json').read_text())['library_sha256']
assert hashlib.sha256(provider.read_bytes()).hexdigest()==json.loads((provider.parent/'report.json').read_text())['provider_sha256']
out=Path(tempfile.mkdtemp(prefix='native-http-observation-',dir=ROOT/'build'))
specs=['cobalt_diag='+str(core),'provider='+str(provider)]
if len(sys.argv)>1:
 resources=Path(sys.argv[1]);report=json.loads((resources/'report.json').read_text())
 assert report['package_sha256']==json.loads((ROOT/'docs/PHASE33_COBALT_AUDIT.json').read_text())['package_sha256']
 for item in report['resources']:
  source=resources/'content'/item['name']
  assert source.stat().st_size==item['bytes'] and hashlib.sha256(source.read_bytes()).hexdigest()==item['sha256']
  specs.append(item['name']+'='+str(source))
 print('Verified pinned resource files:',len(report['resources']),flush=True)
assert len(sys.argv)==3,'Require pinned engine resources and separate platform font package'
font_root=Path(sys.argv[2]);font_report=json.loads((font_root/'report.json').read_text())
for item in font_report['resources']:
 source=font_root/item['name'];data=source.read_bytes()
 assert len(data)==item['bytes'] and hashlib.sha256(data).hexdigest()==item['sha256']
 specs.append('systemfonts/'+item['name']+'='+str(source))
print('Verified separate platform font resources including license:',len(font_report['resources']),flush=True)
disk=out/'assets.img';create(disk,specs)
requests=[]
page=b'''<!doctype html><html><head><meta charset="utf-8"><title>Native MyOS web probe</title>
<style>body{margin:24px;background:#fff0d8;color:#132b43;font:32px sans-serif}button{font:32px sans-serif;padding:20px}</style></head>
<body><h1>MyOS native Cobalt</h1><p id="result">Waiting for JavaScript</p><p id="ticks">Timer 0</p>
<button id="button">Click here</button><p id="input">No input yet</p><script>
var ticks=0,clicks=0,keys=0,images=[];
function report(kind,value){var i=new Image();images.push(i);i.src='/evidence?kind='+kind+'&value='+encodeURIComponent(value)+'&seq='+images.length;}
document.getElementById('result').textContent='Native DOM '+(6*7);report('dom',document.getElementById('result').textContent);
setInterval(function(){ticks++;document.getElementById('ticks').textContent='Timer '+ticks;report('timer',ticks);},1000);
document.getElementById('button').onclick=function(){clicks++;document.getElementById('input').textContent='Clicks '+clicks;report('click',clicks);};
document.onkeydown=function(e){keys++;document.getElementById('input').textContent='Key '+e.key+' / '+keys;report('key',e.key);};
</script></body></html>'''
class Handler(http.server.BaseHTTPRequestHandler):
 def do_GET(self):
  requests.append(dict(path=self.path,elapsed=time.monotonic()-began))
  data=page if self.path=='/' else b''
  self.send_response(200);self.send_header('Content-Type','text/html');self.send_header('Content-Length',str(len(data)))
  self.send_header('Content-Security-Policy',"default-src 'self'; script-src 'self' 'unsafe-inline'; style-src 'self' 'unsafe-inline'; img-src 'self'")
  self.end_headers();self.wfile.write(data)
 def log_message(self,*unused):pass
server=http.server.ThreadingHTTPServer(('127.0.0.1',0),Handler)
began=time.monotonic()
thread=threading.Thread(target=server.serve_forever,daemon=True);thread.start()
vm=VM('cobalt-startup-diagnostic',ram=512,nic=True,asset_disk=disk,cpu='max')
try:
 vm.wait('myos> ');command(vm,'net up','Network: DHCP configured')
 # Disposable guest RAMFS directory; no engine assertions or returns bypassed.
 command(vm,'mkdir /dev/shm','myos> ')
 print('OBSERVATION ONLY: genuine core/provider; native interpreter, no host DOM/JS execution.',out,flush=True)
 offset=len(vm.log.read_text());type_text(vm,f'elf pair-starboard cobalt_diag provider http://10.0.2.2:{server.server_port}/\n')
 deadline=time.monotonic()+240;ready=None;last_capture=-30;sent_input=False
 while time.monotonic()<deadline:
  result=vm.log.read_text()[offset:]
  if 'PANIC' in result or result.endswith('myos> '):break
  now=time.monotonic()
  if ready is None and any('kind=dom&' in item['path'] for item in requests):ready=now
  if now-began-last_capture>=30:
   last_capture=now-began
   vm.command('screendump',{'filename':str(out/f'frame-{int(last_capture):03}.ppm')})
  if ready is not None and now-ready>=5 and not sent_input:
   sent_input=True;vm.key('a')
   # Native QEMU mouse motion/click. Visual inspection must confirm target;
   # a missing click beacon remains failure, never inferred from injection.
   vm.command('input-send-event',{'events':[{'type':'abs','data':{'axis':'x','value':6000}},{'type':'abs','data':{'axis':'y','value':15000}}]})
   for down in (True,False):vm.command('input-send-event',{'events':[{'type':'btn','data':{'down':down,'button':'left'}}]})
  if ready is not None and now-ready>=65:break
  time.sleep(.1)
 (out/'startup.txt').write_text(vm.log.read_text()[offset:])
 vm.command('screendump',{'filename':str(out/'final.ppm')})
 if not vm.log.read_text().endswith('myos> '):
  vm.key('esc');end=time.monotonic()+15
  while time.monotonic()<end and not vm.log.read_text().endswith('myos> '):time.sleep(.1)
 print('Native observation HTTP requests:',requests,flush=True)
 print('No HTTPS/full acceptance claim. Inspect screenshots, beacons and cleanup.',out,flush=True)
finally:
 (out/'requests.json').write_text(json.dumps(requests,indent=2))
 (out/'serial.txt').write_text(vm.log.read_text())
 print('EVIDENCE_BUNDLE',out,'requests',len(requests),flush=True)
 vm.close();server.shutdown();server.server_close();thread.join(timeout=2)
