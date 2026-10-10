"""Authorized bounded native signed-URI diagnostic. Host delivers the original request fixture only; no host media fetching."""
from pathlib import Path
import os,sys,json,time,re,hashlib,datetime,urllib.parse,threading,ssl,http.server,subprocess,shutil
root=Path.cwd();sys.path.insert(0,str(root/'tests'))
from test_system import ROOT,VM
from test_shell import command,type_text
from PIL import Image
captured_path=Path(sys.argv[1]).resolve()
assert captured_path.is_relative_to((root/'build').resolve())
captured=json.loads(captured_path.read_text());uri=captured['uri'];parsed=urllib.parse.urlsplit(uri)
q=urllib.parse.parse_qs(parsed.query);expire=int(q['expire'][0])
assert parsed.scheme=='https' and (parsed.hostname or '').endswith('.googlevideo.com') and parsed.path=='/videoplayback'
assert not parsed.username and not parsed.fragment and '\r' not in uri and '\n' not in uri
assert time.time()-24*3600<expire<time.time()-120
import base64
body=base64.b64decode(captured['body_base64'],validate=True);assert 0<len(body)<=128*1024
assert captured['headers'].get(':method')=='POST'
uri_sha=hashlib.sha256(uri.encode()).hexdigest();body_sha=hashlib.sha256(body).hexdigest();captured_body_bytes=len(body)
allowed={'content-type','content-encoding','accept','x-goog-visitor-id','x-youtube-client-name','x-youtube-client-version','x-goog-api-key'}
headers={k:v for k,v in captured['headers'].items() if k in allowed}
assert all('\r' not in v and '\n' not in v for v in headers.values())
request_json=json.dumps(dict(uri=uri,body=captured['body_base64'],headers=headers,expire=expire,uri_sha256=uri_sha,body_sha256=body_sha)).encode()
probe_config=json.loads((root/'build/phase334-netlog-auaebek6/probe-config.json').read_text())
os.environ['MYOS_RENDERER_PROVIDER_PATH']=str(root/'build/media-evaluation/provider-sey3hj06')
sys.argv=['expiry-native','/home/xytron404/cd-QJYMwG/build/cobalt-resources-dwxalc2n','/home/xytron404/platform-fonts-01W7Ri/build/platform-fonts-20261007']
setup=(root/'tests/test_cobalt_native_acceptance.py').read_text().split('bad={s.label:')[0]
setup=setup.replace("assert provider_directory.is_relative_to((ROOT/'build/renderer-provider-evaluation').resolve())","assert provider_directory.is_relative_to((ROOT/'build/media-evaluation').resolve())")
exec(compile(setup,'native-expiry-sealed-setup','exec'),globals())
page=b"""<!doctype html><html><head><meta charset="utf-8"><style>body{background:#182b3d;color:white;font:22px sans-serif;margin:20px}</style></head><body><h1>MyOS native signed-URL expiry diagnostic</h1><p id="s">Loading original native request</p><p>N: ordinary YouTube watch page</p><script src="/app.js"></script></body></html>"""
script=b"""var reports=[],s=document.getElementById('s');function report(k,v){s.textContent=k+': '+v;var im=new Image();reports.push(im);im.src='/report?kind='+encodeURIComponent(k)+'&value='+encodeURIComponent(v)+'&utc='+encodeURIComponent(new Date().toISOString());}var config=new XMLHttpRequest();config.open('GET','/request.json',true);config.onload=function(){var c=JSON.parse(config.responseText),raw=atob(c.body),body=new Uint8Array(raw.length);for(var i=0;i<raw.length;i++)body[i]=raw.charCodeAt(i);if(window.crypto&&crypto.subtle){crypto.subtle.digest('SHA-256',body).then(function(h){var b=new Uint8Array(h),v='';for(var i=0;i<b.length;i++)v+=('0'+b[i].toString(16)).slice(-2);report('native-body-sha256',v);},function(){report('native-body-sha-unavailable','true');});}function send(stage){var x=new XMLHttpRequest();x.open('POST',c.uri,true);x.timeout=20000;x.responseType='arraybuffer';Object.keys(c.headers).forEach(function(k){x.setRequestHeader(k,c.headers[k]);});x.onload=function(){report(stage+'-response',JSON.stringify({status:x.status,bytes:x.response?x.response.byteLength:0,date:x.getResponseHeader('Date'),body_bytes:body.length,uri_sha256:c.uri_sha256}));};x.onerror=function(){report(stage+'-network-or-cors',JSON.stringify({status:x.status,body_bytes:body.length,uri_sha256:c.uri_sha256}));};x.ontimeout=function(){report(stage+'-timeout',JSON.stringify({status:x.status,body_bytes:body.length,uri_sha256:c.uri_sha256}));};x.send(body);}var deadline=c.expire*1000+120000;report('timer-scheduled',new Date(deadline).toISOString());send('after-expiry');};config.onerror=function(){report('fixture-load-error','true');};config.send();document.addEventListener('keydown',function(e){if(e.keyCode===78){report('restart','ordinary-watch');window.location.href='https://www.youtube.com/watch?v=aqz-KE-bpKQ';}});"""
class ExpiryHandler(http.server.BaseHTTPRequestHandler):
 protocol_version='HTTP/1.1'
 def do_GET(self):
  event=dict(path=self.path,elapsed=time.monotonic()-began)
  with lock:records.append(event)
  body=page if self.path=='/' else script if self.path=='/app.js' else request_json if self.path=='/request.json' else gif
  mime='text/html' if self.path=='/' else 'application/javascript' if self.path=='/app.js' else 'application/json' if self.path=='/request.json' else 'image/gif'
  self.send_response(200);self.send_header('Content-Type',mime);self.send_header('Cache-Control','no-store')
  self.send_header('Content-Security-Policy',"default-src 'self'; script-src 'self'; style-src 'self' 'unsafe-inline'; img-src 'self'; connect-src 'self' https://*.googlevideo.com")
  self.send_header('Content-Length',str(len(body)));self.end_headers()
  try:self.wfile.write(body)
  except (BrokenPipeError,ConnectionResetError):pass
 def log_message(self,*args):pass
for server in servers:
 server.RequestHandlerClass=ExpiryHandler
 thread=threading.Thread(target=server.serve_forever,daemon=True);thread.start();threads.append(thread)
disk=out/'assets.img';create(disk,specs,runtime=root/'build/platform-runtime.mrt')
boot=out/'boot.img';shutil.copy2(Path(probe_config['boot_image']),boot)
sys.path.insert(0,str(root/'docs/wip/native-pmm512/kvm-ata-20261008'))
from qemu_kvm_optin_20261008 import install
install()
qemu=[];original=subprocess.Popen
def capture(args,*pos,**kw):
 if isinstance(args,list) and args and Path(args[0]).name=='qemu-system-x86_64':
  args=list(args);i=args.index('-audiodev')+1;args[i]+=',out.frequency=11025,out.channels=1,out.format=u8';qemu.append(args)
 return original(args,*pos,**kw)
subprocess.Popen=capture
identity=json.loads((root/'build/phase334-carrier-normal-identity.json').read_text())
evidence=dict(phase_passed=False,diagnostic_only=True,date=datetime.datetime.now(datetime.timezone(datetime.timedelta(hours=7))).isoformat(),commit=subprocess.check_output(['git','rev-parse','HEAD'],text=True).strip(),uri_sha256=uri_sha,uri_bytes=len(uri),original_media_hostname=parsed.hostname,expiry_utc=datetime.datetime.fromtimestamp(expire,datetime.timezone.utc).isoformat(),method='POST',request_body_bytes=captured_body_bytes,body_sha256=body_sha,comparable_success_before=False,faithful_sabr_request_proved=False,host_fetch=False,host_decode=False,fixture_request_delivery_only=True,media_hostname_mapping=False,normal_identity=identity,frames=[],inputs=[],requests=records,qemu=qemu)
def wait_new_output(vm,offset,needle,timeout=30,suffix=False):
 end=time.monotonic()+timeout
 while time.monotonic()<end:
  text=vm.log.read_text()[offset:]
  if (text.endswith(needle) if suffix else needle in text):return text
  if 'PANIC' in text:raise RuntimeError('Native panic during new-output wait; private log preserved')
  time.sleep(.05)
 raise RuntimeError('Native new-output wait timed out; private log preserved')

def export_native_file(vm,path,timeout=600):
 import time
 offset=len(vm.log.read_text());type_text(vm,'cat '+path+chr(10));end=time.monotonic()+timeout
 while time.monotonic()<end:
  output=vm.log.read_text()[offset:]
  if output.endswith('myos> '):return output
  time.sleep(.25)
 raise RuntimeError('Native log export exceeded bounded600-second deadline; raw output remains private')

vm=None;restart=None;actions=set()
try:
 vm=VM('phase334-expiry-ump',ram=1024,nic=True,asset_disk=disk,boot_image=boot,audio=out/'audio.wav',cpu='max')
 vm.wait('myos> ');command(vm,'net up','Network: DHCP configured');command(vm,'mkdir /dev/shm','myos> ')
 evidence['before']=command(vm,'mem','Free physical pages: ')
 started=time.monotonic();type_text(vm,'elf cobalt-trace cobalt provider https://cobalt.example.com:'+str(servers[0].server_port)+'/ cobalt.example.com=10.0.2.2\n')
 next_frame=0
 deadline_bound=360
 while time.monotonic()-started<deadline_bound:
  elapsed=time.monotonic()-started
  if elapsed>=next_frame:
   next_frame=elapsed+(5 if restart is not None else 30);ppm=out/('frame-%03d.ppm'%elapsed);vm.command('screendump',{'filename':str(ppm)})
   png=ppm.with_suffix('.png');Image.open(ppm).save(png);evidence['frames'].append(dict(elapsed=elapsed,path=png.name))
  seen=[r for r in records if r['path'].startswith('/report?') and urllib.parse.parse_qs(urllib.parse.urlsplit(r['path']).query).get('kind',[''])[0].startswith('after-expiry-')]
  if seen and restart is None and time.monotonic()-began>seen[0]['elapsed']+15:
   vm.key('n');restart=elapsed;evidence['restart_at']=elapsed;evidence['inputs'].append(dict(key='n',elapsed=elapsed))
  if restart is not None:
   for delay,x,y in []:
    if delay not in actions and elapsed>=restart+delay:
     from test_cobalt_youtube_observation import native_click
     native_click(vm,x,y);actions.add(delay);evidence['inputs'].append(dict(click=[x,y],elapsed=elapsed))
   if elapsed>=restart+75:break
  if vm.log.read_text().rstrip().endswith('myos>'):break
  if ('progress',int(elapsed)//30) not in actions:
   actions.add(('progress',int(elapsed)//30));(out/'safe-progress.json').write_text(json.dumps(dict(uri_sha256=uri_sha,expiry_utc=evidence['expiry_utc'],safe_native_reports=[{k:v[0] for k,v in urllib.parse.parse_qs(urllib.parse.urlsplit(r['path']).query).items()} for r in records if r['path'].startswith('/report?')],remaining_seconds=max(0,expire+120-time.time()),restart_at=restart,report_count=len(seen)),indent=2)+'\n')
  time.sleep(.2)
 close_offset=len(vm.log.read_text());vm.key('esc');wait_new_output(vm,close_offset,'myos> ',timeout=30,suffix=True)
 export_native_file(vm,'/dev/shm/n.json')
 (out/'private-native-netlog-export.txt').write_text(vm.log.read_text())
 command(vm,'rm /dev/shm/n.json','myos> ')
 evidence['after']=command(vm,'mem','Free physical pages: ')
 from test_cobalt_youtube_observation import native_click
 desktop_offset=len(vm.log.read_text());type_text(vm,'desktop'+chr(10))
 wait_new_output(vm,desktop_offset,'DESKTOP READY',timeout=30);native_click(vm,80,105)
 wait_new_output(vm,desktop_offset,'WINDOW OPEN 1'+chr(10)+'DESKTOP FRAME'+chr(10),timeout=20)
 desktop_close_offset=len(vm.log.read_text());vm.key('q');wait_new_output(vm,desktop_close_offset,'myos> ',timeout=30,suffix=True)
 evidence['after_desktop']=command(vm,'mem','Free physical pages: ')
 assert evidence['after_desktop']==evidence['before']
 evidence['desktop_responsive']=True
 evidence['normal_resources_restored']=True
finally:
 if vm:(out/'serial.log').write_text(vm.log.read_text());vm.close()
 for server in servers:server.shutdown();server.server_close()
 evidence['qemu']=qemu;evidence['normal_identity_after']={n:hashlib.sha256((root/'build'/n).read_bytes()).hexdigest() for n in identity}
 assert evidence['normal_identity_after']==identity
 (out/'evidence.json').write_text(json.dumps(evidence,indent=2)+'\n')
 safe=[]
 for r in records:
  if r['path'].startswith('/report?'):
   vals=urllib.parse.parse_qs(urllib.parse.urlsplit(r['path']).query);safe.append({k:v[0] for k,v in vals.items()})
 (out/'safe-native-responses.json').write_text(json.dumps(safe,indent=2)+'\n')
 print('EVIDENCE_BUNDLE',out,'DIAGNOSTIC ONLY; native NetLog/status and playback audit still required')
 print('SAFE_NATIVE_RESPONSES',json.dumps(safe))
