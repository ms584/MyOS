"""Real pinned Cobalt media observation; no automatic phase acceptance.
Uses the audited TLS/resource setup from native-web test, with no verifier bypass.
Host serves bytes and captures framebuffer/audio only; native guest engine decodes.
"""
from pathlib import Path
import os,sys,json,time,re,shutil,hashlib,http.server,ssl,threading,urllib.parse
from test_system import ROOT,VM
from test_shell import command,type_text
from PIL import Image
# Reuse the exact certificate and sealed-resource setup, without running its page
# or its acceptance loop. Keep its identity assertions and trust-store install.
setup=(ROOT/'tests/test_cobalt_native_acceptance.py').read_text().split("bad={s.label:")[0]
setup=setup.replace("assert provider_directory.is_relative_to((ROOT/'build/renderer-provider-evaluation').resolve())",
"assert provider_directory.is_relative_to((ROOT/'build/media-evaluation').resolve())")
exec(compile(setup,'native-web-sealed-tls-setup','exec'),globals())
media=ROOT/'assets/tests/phase333/standard-160x90-40s.webm'
metadata=json.loads((ROOT/'assets/tests/phase333/standard-fixture.json').read_text())
assert hashlib.sha256(media.read_bytes()).hexdigest()==metadata['sha256']
page=b'''<!doctype html><html><head><meta charset="utf-8">
<style>body{background:#182b3d;color:white;font:22px sans-serif;margin:16px}video{width:320px;height:180px;background:black}</style></head>
<body><h1>MyOS native standard VP8 + Opus</h1><p id="status">Loading</p>
<video id="v" width="160" height="90" autoplay></video>
<p>P pause; R resume; S seek 15s; X stop/restart</p><script src="/app.js"></script></body></html>'''
script=b'''var v=document.getElementById('v'),reports=[],sequence=0,ms=new MediaSource(),sb=null,offset=0,fetching=false,finished=false;
function report(k,x){var i=new Image();reports.push(i);if(reports.length>32)reports.shift();i.src='/evidence?kind='+k+'&value='+encodeURIComponent(x)+'&seq='+(++sequence);document.getElementById('status').textContent=k+': '+x;}
report('canplay',v.canPlayType('video/webm; codecs="vp8, opus"'));
report('mse',MediaSource.isTypeSupported('video/webm; codecs="vp8, opus"'));
['loadedmetadata','loadeddata','playing','pause','seeking','seeked','ended','waiting','stalled','error'].forEach(function(k){v.addEventListener(k,function(){report(k,v.currentTime+(v.error?' error='+v.error.code:''));});});
function pump(){
 if(!sb||sb.updating||fetching||finished)return;
 var end=sb.buffered.length?sb.buffered.end(sb.buffered.length-1):0;
 // Bound retained compressed history as well as the5s forward horizon.
 // Serialize eviction and append through SourceBuffer.updateend.
 if(!v.seeking&&v.currentTime>6&&sb.buffered.length&&sb.buffered.start(0)<v.currentTime-5){
  try{sb.remove(0,v.currentTime-3);return;}catch(e){report('remove-error',String(e));return;}
 }
 if(end-v.currentTime>5)return;
 fetching=true;var r=new XMLHttpRequest();r.open('GET','/media.webm');r.responseType='arraybuffer';
 r.setRequestHeader('Range','bytes='+offset+'-'+(offset+8191));
 r.onload=function(){fetching=false;if(r.status!=206){report('fetch-error',r.status);return;}offset+=r.response.byteLength;try{sb.appendBuffer(r.response);}catch(e){report('append-error',String(e));}if(offset>=464222)finished=true;};
 r.onerror=function(){fetching=false;report('fetch-error','network');};r.send();
}
ms.addEventListener('sourceopen',function(){report('sourceopen','ok');try{
 sb=ms.addSourceBuffer('video/webm; codecs="vp8, opus"');
 sb.addEventListener('error',function(){report('sourcebuffer-error','error');});
 sb.addEventListener('updateend',function(){if(finished&&!sb.updating&&ms.readyState=='open'){try{ms.endOfStream();}catch(e){report('eos-error',String(e));}}else pump();});
 pump();v.play();
}catch(e){report('mse-error',String(e));}});
v.src=URL.createObjectURL(ms);
setInterval(function(){report('clock',v.currentTime+' paused='+v.paused+' ready='+v.readyState+' width='+v.videoWidth+' height='+v.videoHeight);pump();},1000);
document.onkeydown=function(e){report('key',e.key);if(e.key=='p')v.pause();if(e.key=='r')v.play();if(e.key=='s')v.currentTime=15;if(e.key=='x'){v.pause();v.removeAttribute('src');v.load();}};
'''
def do_GET(self):
 event=dict(server=self.server.label,path=self.path,range=self.headers.get('Range'),elapsed=time.monotonic()-began)
 with lock:
  records.append(event)
  with (out/'request-events.jsonl').open('a') as f:f.write(json.dumps(event)+'\n')
 try:
  if self.path=='/media.webm':
   start=0;end=media.stat().st_size-1;request=self.headers.get('Range')
   if request:
    match=re.fullmatch(r'bytes=(\d+)-(\d*)',request)
    if not match:self.send_error(416);return
    start=int(match[1]);end=min(end,int(match[2]) if match[2] else end)
    if start>end:self.send_error(416);return
   self.send_response(206 if request else 200)
   self.send_header('Content-Type','video/webm');self.send_header('Accept-Ranges','bytes')
   self.send_header('Content-Length',str(end-start+1))
   if request:self.send_header('Content-Range',f'bytes {start}-{end}/{media.stat().st_size}')
   self.end_headers()
   with media.open('rb') as f:
    f.seek(start);remain=end-start+1
    while remain:
     chunk=f.read(min(4096,remain));self.wfile.write(chunk);self.wfile.flush();remain-=len(chunk);time.sleep(.01)
   return
  body=page if self.path=='/' else script if self.path=='/app.js' else gif
  self.send_response(200);self.send_header('Content-Type','text/html' if self.path=='/' else 'application/javascript' if self.path=='/app.js' else 'image/gif')
  self.send_header('Content-Security-Policy',"default-src 'self'; script-src 'self'; style-src 'self' 'unsafe-inline'; media-src 'self' blob:; img-src 'self'")
  self.send_header('Content-Length',str(len(body)));self.send_header('Cache-Control','no-store')
  self.end_headers();self.wfile.write(body)
 except (BrokenPipeError,ConnectionResetError,ssl.SSLError):pass
Handler.do_GET=do_GET
runtime_source=Path(os.environ.get('MYOS_MEDIA_RUNTIME_PATH',str(ROOT/'build/platform-runtime.mrt'))).resolve()
runtime_copy=out/'platform-runtime.mrt';shutil.copyfile(runtime_source,runtime_copy)
runtime_hash=hashlib.sha256(runtime_copy.read_bytes()).hexdigest()
disk=out/'assets.img';create(disk,specs,runtime=runtime_copy)
# Freeze both image and runtime before starting; independent system suite may build
# different images. The VM and disk contain these immutable copies.
boot=out/'boot.img';shutil.copyfile(Path(os.environ.get('MYOS_MEDIA_BOOT_IMAGE',str(ROOT/'build/myos-current.img'))),boot)
boot_hash=hashlib.sha256(boot.read_bytes()).hexdigest()
for server in servers:
 thread=threading.Thread(target=server.serve_forever,daemon=True);thread.start();threads.append(thread)
sys.path.insert(0,str(ROOT/'docs/wip/native-pmm512/kvm-ata-20261008'))
from qemu_kvm_optin_20261008 import install
install()
vm=None;frames=[];inputs=[];started=time.monotonic();playing=None;sent=set()
try:
 vm=VM('cobalt-media-native',ram=512,nic=True,asset_disk=disk,boot_image=boot,audio=out/'audio.wav',cpu='max')
 vm.wait('myos> ');command(vm,'net up','Network: DHCP configured');command(vm,'mkdir /dev/shm','myos> ')
 offset=len(vm.log.read_text());type_text(vm,'elf cobalt cobalt provider https://cobalt.example.com:'+str(servers[0].server_port)+'/ cobalt.example.com=10.0.2.2\n')
 deadline=time.monotonic()+300;next_frame=0
 while time.monotonic()<deadline:
  elapsed=time.monotonic()-started;content=vm.log.read_text()[offset:]
  if 'USER FAULT' in content or 'PANIC' in content or content.endswith('myos> '):break
  if elapsed>=next_frame:
   next_frame=elapsed+2;p=out/('media-%03d.ppm'%elapsed)
   vm.command('screendump',{'filename':str(p)});png=p.with_suffix('.png');Image.open(p).save(png);p.unlink()
   frames.append(dict(elapsed=elapsed,path=png.name))
  observations=[urllib.parse.parse_qs(urllib.parse.urlsplit(r['path']).query) for r in records if r['path'].startswith('/evidence?')]
  if any(o.get('kind')==['playing'] for o in observations) and playing is None:playing=time.monotonic()
  if any(o.get('kind')==['error'] for o in observations):print('ENGINE MEDIA ERROR; preserving failed evidence',flush=True);break
  if playing:
   delta=time.monotonic()-playing
   for when,key in ((6,'p'),(9,'r'),(15,'s'),(44,'x')):
    if delta>=when and key not in sent:
     vm.key(key);sent.add(key);inputs.append(dict(key=key,elapsed=elapsed))
   if delta>=48:break
  time.sleep(.1)
 type_text(vm,'\x1b') if False else vm.key('esc')
 try:vm.wait('myos> ',timeout=15)
 except AssertionError:pass
finally:
 if vm:
  (out/'serial.log').write_text(vm.log.read_text());vm.close()
 for server in servers:server.shutdown();server.server_close()
 for thread in threads:thread.join(timeout=2)
 assert hashlib.sha256(boot.read_bytes()).hexdigest()==boot_hash
 (out/'evidence.json').write_text(json.dumps(dict(phase_passed=False,observation_only=True,diagnostic_trace=json.loads((provider.parent/'report.json').read_text()).get('diagnostic_trace',False),
 requests=records,frames=frames,inputs=inputs,provider_sha256=hashlib.sha256(provider.read_bytes()).hexdigest(),
 fixture_sha256=metadata['sha256'],boot_sha256=boot_hash,runtime_sha256=runtime_hash,elapsed=time.monotonic()-started),indent=2)+'\n')
 print('EVIDENCE_BUNDLE',out,'NOT automatic phase acceptance',flush=True)
