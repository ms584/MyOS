"""Failure-only call probes: NOT completed APIs or web acceptance evidence."""
from pathlib import Path
import hashlib
import http.server
import json
import subprocess
import sys
import tempfile
import threading
from test_system import ROOT,VM
from test_shell import command
from test_elf_dynamic import long_command
sys.path.insert(0,str(ROOT/'tools'))
from make_asset_disk import create
core=ROOT/'build/engine-evaluation/libcobalt.so'
provider=ROOT/'build/renderer-provider-evaluation/provider-84sokh6g/provider.so'
assert hashlib.sha256(core.read_bytes()).hexdigest()==json.loads((ROOT/'docs/PHASE33_COBALT_AUDIT.json').read_text())['library_sha256']
assert hashlib.sha256(provider.read_bytes()).hexdigest()==json.loads((provider.parent/'report.json').read_text())['provider_sha256']
out=Path(tempfile.mkdtemp(prefix='startup-diagnostic-',dir=ROOT/'build'))
specs=['cobalt_diag='+str(core),'provider='+str(provider)]
if len(sys.argv)>1:
 resources=Path(sys.argv[1]);report=json.loads((resources/'report.json').read_text())
 assert report['package_sha256']==json.loads((ROOT/'docs/PHASE33_COBALT_AUDIT.json').read_text())['package_sha256']
 for item in report['resources']:
  source=resources/'content'/item['name']
  assert source.stat().st_size==item['bytes'] and hashlib.sha256(source.read_bytes()).hexdigest()==item['sha256']
  specs.append(item['name']+'='+str(source))
 print('Verified pinned resource files:',len(report['resources']),flush=True)
disk=out/'assets.img';create(disk,specs)
requests=[]
class Handler(http.server.BaseHTTPRequestHandler):
 def do_GET(self):
  requests.append(self.path)
  data=b'<!doctype html><html><body><p id="result">starting</p><script>document.getElementById("result").textContent="native DOM "+(6*7);</script></body></html>'
  self.send_response(200);self.send_header('Content-Type','text/html');self.send_header('Content-Length',str(len(data)));self.end_headers();self.wfile.write(data)
 def log_message(self,*unused):pass
server=http.server.ThreadingHTTPServer(('127.0.0.1',0),Handler)
thread=threading.Thread(target=server.serve_forever,daemon=True);thread.start()
vm=VM('cobalt-startup-diagnostic',ram=512,nic=True,asset_disk=disk,cpu='max')
try:
 vm.wait('myos> ');command(vm,'net up','Network: DHCP configured')
 # Disposable guest RAMFS directory; no engine assertions or returns bypassed.
 command(vm,'mkdir /dev/shm','myos> ')
 print('DIAGNOSTIC ONLY: genuine Cobalt/provider; remaining unsupported calls trap rather than succeed.',flush=True)
 result=long_command(vm,f'elf pair-starboard cobalt_diag provider http://10.0.2.2:{server.server_port}/',timeout=240)
 (out/'startup.txt').write_text(result)
 print('\n'.join(line for line in result.splitlines() if any(marker in line for marker in
  ('SABI','DIAGNOSTIC','USER FAULT','missing=','started','heap-before','ELF exited','unsupported','memory'))),flush=True)
 print('Controlled fixture HTTP requests:',requests,flush=True)
 print('DIAGNOSTIC RESULT ONLY; production coverage unchanged and Cobalt acceptance NOT PASSED.',out,flush=True)
finally:
 vm.close();server.shutdown();server.server_close();thread.join(timeout=2)
