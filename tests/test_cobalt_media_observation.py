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
<p>P pause; R resume; S seek 15s; X stop; N restart</p><script src="/app.js"></script></body></html>'''
script=b'''var v=document.getElementById('v'),reports=[],sequence=0,ms=null,sb=null,offset=0,fetching=false,finished=false,generation=0,inflight=null,objectURL=null;
function report(k,x){var i=new Image();reports.push(i);if(reports.length>32)reports.shift();i.src='/evidence?kind='+k+'&value='+encodeURIComponent(x)+'&seq='+(++sequence);document.getElementById('status').textContent=k+': '+x;}
report('canplay',v.canPlayType('video/webm; codecs="vp8, opus"'));
report('mse',MediaSource.isTypeSupported('video/webm; codecs="vp8, opus"'));
['loadedmetadata','loadeddata','playing','pause','seeking','seeked','ended','waiting','stalled','error'].forEach(function(k){v.addEventListener(k,function(){report(k,v.currentTime+' generation='+generation+(v.error?' error='+v.error.code:''));});});
function stop(){
 ++generation;
 if(inflight){inflight.abort();inflight=null;}
 fetching=false;finished=false;sb=null;ms=null;
 v.pause();v.removeAttribute('src');v.load();
 if(objectURL){URL.revokeObjectURL(objectURL);objectURL=null;}
 report('stopped',generation);
}
function pump(){
 if(!sb||sb.updating||fetching||finished)return;
 var end=sb.buffered.length?sb.buffered.end(sb.buffered.length-1):0;
 // Keep three seconds behind and at most five seconds ahead. Eviction and
 // append are serialized by updateend, with one bounded HTTP request.
 if(!v.seeking&&v.currentTime>6&&sb.buffered.length&&sb.buffered.start(0)<v.currentTime-5){
  try{sb.remove(0,v.currentTime-3);return;}catch(e){report('remove-error',String(e));return;}
 }
 if(end-v.currentTime>5)return;
 fetching=true;var r=new XMLHttpRequest(),epoch=generation,target=sb;
 inflight=r;r.open('GET','/media.webm');r.responseType='arraybuffer';r.timeout=10000;
 r.setRequestHeader('Range','bytes='+offset+'-'+(offset+8191));
 r.onload=function(){
  if(epoch!=generation)return;
  fetching=false;inflight=null;
  if(r.status!=206||!r.response||!r.response.byteLength){report('fetch-error',r.status);return;}
  offset+=r.response.byteLength;
  try{target.appendBuffer(r.response);}catch(e){report('append-error',String(e));}
  if(offset>=464222)finished=true;
 };
 r.onerror=r.ontimeout=function(){if(epoch!=generation)return;fetching=false;inflight=null;report('fetch-error','network');};
 r.send();
}
function begin(){
 stop();offset=0;var epoch=generation,source=new MediaSource();ms=source;
 source.addEventListener('sourceopen',function(){
  if(epoch!=generation)return;
  report('sourceopen','generation='+epoch);
  try{
   var target=source.addSourceBuffer('video/webm; codecs="vp8, opus"');sb=target;
   target.addEventListener('error',function(){if(epoch==generation)report('sourcebuffer-error','error');});
   target.addEventListener('updateend',function(){
    if(epoch!=generation)return;
    if(finished&&!target.updating&&source.readyState=='open'){try{source.endOfStream();report('eos-submitted',offset);}catch(e){report('eos-error',String(e));}}
    else pump();
   });
   pump();v.play();
  }catch(e){report('mse-error',String(e));}
 });
 objectURL=URL.createObjectURL(source);v.src=objectURL;report('started',epoch);
}
begin();
setInterval(function(){report('clock',v.currentTime+' paused='+v.paused+' ready='+v.readyState+' width='+v.videoWidth+' height='+v.videoHeight+' generation='+generation);pump();},1000);
document.onkeydown=function(e){report('key',e.key);if(e.key=='p')v.pause();if(e.key=='r')v.play();if(e.key=='s')v.currentTime=15;if(e.key=='x')stop();if(e.key=='n')begin();};
'''
ram_diagnostic=os.environ.get('MYOS_MEDIA_RAM_DIAGNOSTIC')=='1'
if ram_diagnostic:
 script=script.replace(b'begin();\nsetInterval',b"report('media-ready','awaiting B');\nsetInterval")
 script=script.replace(b"if(e.key=='n')begin();",b"if(e.key=='n'||e.key=='b')begin();")
fault_mode=os.environ.get('MYOS_MEDIA_FAULT','')
assert fault_mode in ('','stall','truncate','corrupt','close','oom')
fault_injections=[]
if fault_mode=='oom':
 script=script.replace(b"if(e.key=='p')v.pause();",b'''if(e.key=='o'){
  var pressure=[],failed='',bytes=0;
  try{for(var i=0;i<32;++i){var block=new ArrayBuffer(16777216),touch=new Uint8Array(block);
   for(var j=0;j<touch.length;j+=4096)touch[j]=1;
   pressure.push(block);bytes+=block.byteLength;}}
  catch(err){failed=String(err);}
  pressure=null;touch=null;block=null;
  report(failed?'oom-rejected':'oom-no-rejection',bytes+' '+failed);
 }if(e.key=='p')v.pause();''')
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
   inject=False
   with lock:
    if fault_mode in ('stall','truncate','corrupt') and not fault_injections and (fault_mode=='corrupt' or start>=131072):
     inject=True;fault_injections.append(dict(mode=fault_mode,start=start,elapsed=time.monotonic()-began))
   if inject and fault_mode=='stall':time.sleep(8)
   self.send_response(206 if request else 200)
   self.send_header('Content-Type','video/webm');self.send_header('Accept-Ranges','bytes')
   self.send_header('Content-Length',str(end-start+1))
   if request:self.send_header('Content-Range',f'bytes {start}-{end}/{media.stat().st_size}')
   self.end_headers()
   with media.open('rb') as f:
    f.seek(start);remain=end-start+1
    while remain:
     chunk=f.read(min(4096,remain))
     if inject and fault_mode=='corrupt':chunk=bytes(len(chunk))
     if inject and fault_mode=='truncate':
      self.wfile.write(chunk[:max(1,len(chunk)//2)]);self.wfile.flush()
      self.close_connection=True;return
     self.wfile.write(chunk);self.wfile.flush();remain-=len(chunk);time.sleep(.01)
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
native_pcm_capture=os.environ.get('MYOS_MEDIA_NATIVE_PCM_CAPTURE')=='1'
if os.environ.get('MYOS_MEDIA_QEMU_INTERRUPTS')=='1' or native_pcm_capture:
 original_popen=subprocess.Popen
 def diagnostic_popen(args,*positional,**kwargs):
  if isinstance(args,list) and args and Path(args[0]).name=='qemu-system-x86_64':
   args=list(args)
   if native_pcm_capture:
    index=args.index('-audiodev')+1
    args[index]+=',out.frequency=11025,out.channels=1,out.format=u8'
   if os.environ.get('MYOS_MEDIA_QEMU_INTERRUPTS')=='1':args+=[ '-d','int','-D',str(out/'private-qemu-interrupts.log')]
  return original_popen(args,*positional,**kwargs)
 subprocess.Popen=diagnostic_popen
heap_diagnostic=os.environ.get('MYOS_MEDIA_HEAP_DIAGNOSTIC')=='1'
def heap_snapshot(label):
 # QMP reads guest virtual kernel heap only before launch/after return to shell.
 # Raw private snapshots stay in build; publish allocation sizes/hashes only.
 raw=out/('private-heap-'+label+'.bin')
 result=vm.command('human-monitor-command',{'command-line':f'memsave 0x100000000 0x1000000 "{raw}"'})
 assert not result,result
 data=raw.read_bytes();blocks=[];cursor=0;visited=set();inode_owners={}
 kernel_elf=os.environ.get('MYOS_MEDIA_KERNEL_ELF')
 if kernel_elf:
  from test_system import parse_symbols
  syms=parse_symbols(subprocess.check_output(['nm',kernel_elf],text=True))
  assert 'nodes' in syms
  nodes_data=vm.memory(syms['nodes'],128*80)
  struct=__import__('struct');nodes=[]
  for n in range(128):
   row=nodes_data[n*80:(n+1)*80]
   used,unlinked=struct.unpack_from('<BB',row)
   parent,kind,refs,size,capacity=struct.unpack_from('<IIIQQ',row,4)
   name=row[32:64].split(b'\0',1)[0].decode('ascii','replace')
   address,shared=struct.unpack_from('<QQ',row,64)
   nodes.append(dict(used=used,unlinked=unlinked,parent=parent,kind=kind,refs=refs,size=size,capacity=capacity,name=name,data=address,shared=shared))
  def inode_path(n):
   parts=[];seen=set()
   while n:
    assert n not in seen and n<128;seen.add(n);parts.append(nodes[n]['name']);n=nodes[n]['parent']
   return '/'+('/'.join(reversed(parts)))
  for n,node in enumerate(nodes):
   if node['used'] and node['data']:
    inode_owners[node['data']]=dict(path=inode_path(n),size=node['size'],capacity=node['capacity'],refs=node['refs'],unlinked=node['unlinked'])
  (out/('inodes-'+label+'.json')).write_text(json.dumps([dict(node=n,**node) for n,node in enumerate(nodes) if node['used']],indent=2)+'\n')

 while True:
  assert cursor not in visited and 0<=cursor<=len(data)-32
  visited.add(cursor);size,nxt,prev,free=__import__('struct').unpack_from('<QQQQ',data,cursor)
  assert free in (0,1) and size<=len(data)-cursor-32
  if not free:blocks.append(dict(offset=cursor,size=size,sha256=hashlib.sha256(data[cursor+32:cursor+32+size]).hexdigest(),inode=inode_owners.get(0x100000000+cursor+32)))
  if not nxt:break
  cursor=nxt-0x100000000
 (out/('heap-'+label+'.json')).write_text(json.dumps(blocks,indent=2)+'\n')
 return blocks
ram_samples=[];ram_baselines=[]
ram_symbol=None;ram_extra_symbols={}
if ram_diagnostic:
 from test_system import parse_symbols
 ram_symbols=parse_symbols(subprocess.check_output(['nm',os.environ['MYOS_MEDIA_KERNEL_ELF']],text=True))
 ram_symbol=ram_symbols['free_count']
 for name in ('heap_live_bytes','heap_runtime_peak_bytes','irq_count','underrun_count','consumed_frames'):
  # parse_symbols already adds an alias only for a unique LTO owner.
  if name in ram_symbols:ram_extra_symbols[name]=ram_symbols[name]
def sample_ram(launch,label):
 free=__import__('struct').unpack('<Q',vm.memory(ram_symbol,8))[0]
 row=dict(launch=launch,elapsed=time.monotonic()-started,label=label,free_pages=free)
 for name,address in ram_extra_symbols.items():row[name]=__import__('struct').unpack('<Q',vm.memory(address,8))[0]
 ram_samples.append(row);return row
vm=None;frames=[];inputs=[];started=time.monotonic();playing=None;sent=set()
try:
 vm=VM('cobalt-media-native',ram=512,nic=True,asset_disk=disk,boot_image=boot,audio=out/'audio.wav',cpu='max')
 vm.wait('myos> ');command(vm,'net up','Network: DHCP configured');command(vm,'mkdir /dev/shm','myos> ')
 repeats=int(os.environ.get('MYOS_MEDIA_REPEATS','1'));assert 1<=repeats<=3
 lifecycle=os.environ.get('MYOS_MEDIA_LIFECYCLE')=='1'
 for launch in range(repeats):
  sent=set();playing=None;fault_seen=None;recovery_at=None;ended_at=None;restarted_at=None;record_start=len(records);suffix='' if not launch else '-launch'+str(launch)
  if heap_diagnostic:heap_snapshot('before'+suffix)
  offset=len(vm.log.read_text());type_text(vm,'elf cobalt cobalt provider https://cobalt.example.com:'+str(servers[0].server_port)+'/ cobalt.example.com=10.0.2.2\n')
  deadline=time.monotonic()+300;next_frame=0;next_ram=0;gate_at=None;baseline=None
  while time.monotonic()<deadline:
   elapsed=time.monotonic()-started;content=vm.log.read_text()[offset:]
   if 'USER FAULT' in content or 'PANIC' in content or content.endswith('myos> '):break
   if elapsed>=next_frame:
    next_frame=elapsed+2;p=out/('media-%03d.ppm'%elapsed)
    vm.command('screendump',{'filename':str(p)});png=p.with_suffix('.png');Image.open(p).save(png);p.unlink()
    frames.append(dict(launch=launch,elapsed=elapsed,path=png.name))
   observations=[urllib.parse.parse_qs(urllib.parse.urlsplit(r['path']).query) for r in records[record_start:] if r['path'].startswith('/evidence?')]
   if ram_diagnostic and baseline is None:
    if gate_at is None and any(o.get('kind')==['media-ready'] for o in observations):gate_at=time.monotonic()
    if gate_at and time.monotonic()-gate_at>=3:
     baseline=sample_ram(launch,'baseline');ram_baselines.append(baseline)
     vm.key('b');inputs.append(dict(launch=launch,key='b',elapsed=elapsed))
   if ram_diagnostic and baseline and elapsed>=next_ram:
    sample_ram(launch,'active');next_ram=elapsed+.5
   if any(o.get('kind')==['playing'] for o in observations) and playing is None:playing=time.monotonic()
   errors=[o for o in observations if o.get('kind',[''])[0] in ('error','fetch-error','append-error','sourcebuffer-error')]
   if fault_mode:
    if errors and fault_seen is None:fault_seen=time.monotonic()
    if fault_seen and time.monotonic()-fault_seen>=2 and recovery_at is None:
     vm.key('n');recovery_at=time.monotonic();inputs.append(dict(launch=launch,key='n',elapsed=elapsed))
    if fault_mode!='oom' and recovery_at and time.monotonic()-recovery_at>=18:
     vm.key('x');inputs.append(dict(launch=launch,key='x',elapsed=elapsed));time.sleep(2);break
    if fault_mode=='oom' and playing:
     if time.monotonic()-playing>=8 and 'o' not in sent:
      vm.key('o');sent.add('o');recovery_at=time.monotonic();inputs.append(dict(launch=launch,key='o',elapsed=elapsed))
     if recovery_at and time.monotonic()-recovery_at>=12:break
    if fault_mode=='close' and playing and time.monotonic()-playing>=8:break
    if fault_mode=='stall' and playing and time.monotonic()-playing>=32:
     vm.key('x');inputs.append(dict(launch=launch,key='x',elapsed=elapsed));time.sleep(2);break
   elif errors:print('ENGINE MEDIA ERROR; preserving failed evidence',flush=True);break
   if playing:
    delta=time.monotonic()-playing
    for when,key in (() if fault_mode else ((6,'p'),(9,'r'),(15,'s')) if lifecycle else ((6,'p'),(9,'r'),(15,'s'),(44,'x'))):
     if delta>=when and key not in sent:
      vm.key(key);sent.add(key);inputs.append(dict(launch=launch,key=key,elapsed=elapsed))
    if lifecycle:
     if ended_at is None and any(o.get('kind')==['ended'] for o in observations):ended_at=time.monotonic()
     if ended_at and time.monotonic()-ended_at>=2 and 'n' not in sent:
      vm.key('n');sent.add('n');restarted_at=time.monotonic();inputs.append(dict(launch=launch,key='n',elapsed=elapsed))
     if restarted_at and time.monotonic()-restarted_at>=12 and 'x' not in sent:
      vm.key('x');sent.add('x');inputs.append(dict(launch=launch,key='x',elapsed=elapsed))
     if restarted_at and time.monotonic()-restarted_at>=16:break
     if delta>=100:break
    elif delta>=48:break
   time.sleep(.1)
  type_text(vm,'\x1b') if False else vm.key('esc')
  try:
   vm.wait('myos> ',timeout=15)
   if heap_diagnostic:
    heap_snapshot('after'+suffix)
    time.sleep(3);heap_snapshot('after-3s'+suffix)
  except AssertionError:pass
finally:
 if vm:
  (out/'serial.log').write_text(vm.log.read_text());vm.close()
 for server in servers:server.shutdown();server.server_close()
 for thread in threads:thread.join(timeout=2)
 assert hashlib.sha256(boot.read_bytes()).hexdigest()==boot_hash
 (out/'evidence.json').write_text(json.dumps(dict(phase_passed=False,observation_only=True,diagnostic_trace=json.loads((provider.parent/'report.json').read_text()).get('diagnostic_trace',False),
 fault_mode=fault_mode,fault_injections=fault_injections,native_pcm_capture=native_pcm_capture,ram_diagnostic=ram_diagnostic,ram_samples=ram_samples,ram_baselines=ram_baselines,lifecycle_scenario=os.environ.get('MYOS_MEDIA_LIFECYCLE')=='1',requests=records,frames=frames,inputs=inputs,provider_sha256=hashlib.sha256(provider.read_bytes()).hexdigest(),
 fixture_sha256=metadata['sha256'],boot_sha256=boot_hash,runtime_sha256=runtime_hash,elapsed=time.monotonic()-started),indent=2)+'\n')
 print('EVIDENCE_BUNDLE',out,'NOT automatic phase acceptance',flush=True)
