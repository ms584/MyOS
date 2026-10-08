"""Real Cobalt/BoringSSL HTTPS observation, NOT automatic full acceptance.

The host serves TLS fixtures only. A disposable public CA is installed through
the engine's ordinary ssl/certs asset directory. No verifier/policy bypass.
Negative servers must receive no application HTTP requests. Private keys remain
in the private build directory and must never be copied into evidence commits.
"""
from pathlib import Path
import base64, datetime, hashlib, http.server, ipaddress, json, os
import ssl, subprocess, sys, tempfile, threading, time, urllib.parse, re
from cryptography import x509
from cryptography.hazmat.primitives import hashes, serialization
from cryptography.hazmat.primitives.asymmetric import rsa
from cryptography.x509.oid import NameOID, ExtendedKeyUsageOID
from test_system import ROOT, VM
from test_shell import command, type_text
sys.path.insert(0,str(ROOT/'tools'))
from make_asset_disk import create

assert '--https-enforcement=false' not in (ROOT/'memory/process.c').read_text(), 'HTTPS policy override still present'
assert '--host-resolver-rules=MAP cobalt.example.com 10.0.2.2' in (ROOT/'memory/process.c').read_text()
core=ROOT/'build/engine-evaluation/libcobalt.so'
provider=ROOT/'build/renderer-provider-evaluation/provider-84sokh6g/provider.so'
audit=json.loads((ROOT/'docs/PHASE33_COBALT_AUDIT.json').read_text())
assert hashlib.sha256(core.read_bytes()).hexdigest()==audit['library_sha256']
assert hashlib.sha256(provider.read_bytes()).hexdigest()==json.loads((provider.parent/'report.json').read_text())['provider_sha256']
assert len(sys.argv)==3,'Require pinned engine resource and platform font directories'
out=Path(tempfile.mkdtemp(prefix='native-verified-tls-',dir=ROOT/'build'))
os.umask(0o077)
specs=['cobalt_diag='+str(core),'provider='+str(provider)]
resources=Path(sys.argv[1]);report=json.loads((resources/'report.json').read_text())
assert report['package_sha256']==audit['package_sha256']
for item in report['resources']:
    source=resources/'content'/item['name'];data=source.read_bytes()
    assert len(data)==item['bytes'] and hashlib.sha256(data).hexdigest()==item['sha256']
    specs.append(item['name']+'='+str(source))
fonts=Path(sys.argv[2])
for item in json.loads((fonts/'report.json').read_text())['resources']:
    source=fonts/item['name'];data=source.read_bytes()
    assert len(data)==item['bytes'] and hashlib.sha256(data).hexdigest()==item['sha256']
    specs.append('systemfonts/'+item['name']+'='+str(source))

now=datetime.datetime.now(datetime.timezone.utc)
def key():return rsa.generate_private_key(public_exponent=65537,key_size=2048)
def authority(label):
    private=key();name=x509.Name([x509.NameAttribute(NameOID.COMMON_NAME,label)])
    cert=(x509.CertificateBuilder().subject_name(name).issuer_name(name)
        .public_key(private.public_key()).serial_number(x509.random_serial_number())
        .not_valid_before(now-datetime.timedelta(days=1)).not_valid_after(now+datetime.timedelta(days=7))
        .add_extension(x509.BasicConstraints(ca=True,path_length=0),critical=True)
        .add_extension(x509.KeyUsage(False,False,False,False,False,True,True,False,False),critical=True)
        .add_extension(x509.SubjectKeyIdentifier.from_public_key(private.public_key()),critical=False)
        .sign(private,hashes.SHA256()))
    return private,cert
ca_key,ca=authority('MyOS disposable native HTTPS fixture CA')
other_key,other_ca=authority('MyOS deliberately untrusted fixture CA')
ca_file=out/'public-test-ca.pem';ca_file.write_bytes(ca.public_bytes(serialization.Encoding.PEM))
ca_hash=subprocess.check_output(['openssl','x509','-in',str(ca_file),'-subject_hash','-noout'],text=True).strip()
assert len(ca_hash)==8 and all(c in '0123456789abcdef' for c in ca_hash)
assert not any(s.startswith('ssl/certs/'+ca_hash+'.0=') for s in specs)
specs.append('ssl/certs/'+ca_hash+'.0='+str(ca_file))

records=[];lock=threading.Lock();active=0;peak=0;servers=[];threads=[]
began=time.monotonic()
gif=base64.b64decode('R0lGODlhAQABAIAAAAAAAP///yH5BAEAAAAALAAAAAABAAEAAAIBRAA7')
page=b'';script=b''
class Handler(http.server.BaseHTTPRequestHandler):
    protocol_version='HTTP/1.1'
    def do_GET(self):
        global active,peak
        asset=self.path.startswith('/asset/')
        with lock:
            event=dict(server=self.server.label,path=self.path,elapsed=time.monotonic()-began)
            records.append(event)
            # Host observation only; make long-running real guest progress
            # inspectable without attaching to or changing the guest engine.
            with (out/'request-events.jsonl').open('a') as journal:
                journal.write(json.dumps(event)+'\n')
            if asset:active+=1;peak=max(peak,active)
        try:
            if asset:time.sleep(2)
            body=page if self.path=='/' else script if self.path=='/app.js' else gif if asset or self.path=='/probe' else b''
            mime='text/html' if self.path=='/' else 'application/javascript' if self.path=='/app.js' else 'image/gif' if asset or self.path=='/probe' else 'text/plain'
            self.send_response(200);self.send_header('Content-Type',mime)
            # Cobalt enforces CSP on navigation. Permit only fixture content;
            # alternate HTTPS ports are needed for real certificate negatives.
            self.send_header('Content-Security-Policy',
                "default-src 'self'; script-src 'self'; style-src 'self' 'unsafe-inline'; "
                "img-src 'self' https://cobalt.example.com:*")
            self.send_header('Content-Length',str(len(body)));self.send_header('Cache-Control','no-store')
            self.end_headers();self.wfile.write(body)
        except (BrokenPipeError,ConnectionResetError,ssl.SSLError):pass
        finally:
            if asset:
                with lock:active-=1
    def log_message(self,*unused):pass
for label in ('valid','wrong-host','expired','untrusted'):
    private=key();issuer_key,issuer=(other_key,other_ca) if label=='untrusted' else (ca_key,ca)
    host='wrong.example.com' if label=='wrong-host' else 'cobalt.example.com'
    start=now-datetime.timedelta(days=30 if label=='expired' else 1)
    end=now+datetime.timedelta(days=-1 if label=='expired' else 2)
    cert=(x509.CertificateBuilder().subject_name(x509.Name([x509.NameAttribute(NameOID.COMMON_NAME,host)]))
        .issuer_name(issuer.subject).public_key(private.public_key()).serial_number(x509.random_serial_number())
        .not_valid_before(start).not_valid_after(end)
        .add_extension(x509.BasicConstraints(ca=False,path_length=None),critical=True)
        .add_extension(x509.SubjectAlternativeName([x509.DNSName(host)]),critical=False)
        .add_extension(x509.ExtendedKeyUsage([ExtendedKeyUsageOID.SERVER_AUTH]),critical=False)
        .add_extension(x509.KeyUsage(True,False,True,False,False,False,False,False,False),critical=True)
        .add_extension(x509.AuthorityKeyIdentifier.from_issuer_public_key(issuer_key.public_key()),critical=False)
        .sign(issuer_key,hashes.SHA256()))
    cert_file=out/(label+'.pem');key_file=out/(label+'.private-key.pem')
    cert_file.write_bytes(cert.public_bytes(serialization.Encoding.PEM))
    key_file.write_bytes(private.private_bytes(serialization.Encoding.PEM,serialization.PrivateFormat.PKCS8,serialization.NoEncryption()))
    server=http.server.ThreadingHTTPServer(('127.0.0.1',0),Handler);server.daemon_threads=True;server.label=label
    context=ssl.SSLContext(ssl.PROTOCOL_TLS_SERVER);context.load_cert_chain(cert_file,key_file)
    server.socket=context.wrap_socket(server.socket,server_side=True)
    servers.append(server)
bad={s.label:'https://cobalt.example.com:'+str(s.server_port)+'/probe' for s in servers[1:]}
page=b'''<!doctype html><html><head><meta charset="utf-8"><title>Native verified HTTPS</title>
<style>body{margin:24px;background:#d8f0ff;color:#132b43;font:32px sans-serif}button{font:32px sans-serif;padding:20px}</style></head>
<body><h1>Native verified HTTPS</h1><p id="dom">Waiting for external JavaScript</p><p id="ticks">Timer 0</p>
<button id="button">Click here</button><p id="input">No input yet</p><p id="tls">Checking certificates</p>
<div id="scrollbox" style="position:fixed;left:24px;right:24px;bottom:8px;height:60px;overflow:auto;border:2px solid #132b43"><div style="height:1000px">Scroll TOP<br><br><br>Scroll MIDDLE<br><br><br>Scroll BOTTOM</div></div>
<script src="/app.js"></script></body></html>'''
script=('''var images=[],ticks=0,clicks=0,checks=0,pointer=null,pointerSeq=0,lastPointer=0;
function report(kind,value){var i=new Image();images.push(i);i.src='/evidence?kind='+kind+'&value='+encodeURIComponent(value)+'&seq='+images.length;}
document.getElementById('dom').textContent='HTTPS DOM '+(6*7);report('dom','42');
setInterval(function(){ticks++;document.getElementById('ticks').textContent='Timer '+ticks;report('timer',ticks);},1000);
document.getElementById('button').onclick=function(){clicks++;document.getElementById('input').textContent='Clicks '+clicks;report('click',clicks);};
document.onmousemove=function(e){pointer={x:e.clientX,y:e.clientY,seq:++pointerSeq};};
setInterval(function(){if(pointer&&lastPointer!==pointer.seq){lastPointer=pointer.seq;report('pointer',JSON.stringify(pointer));}},500);
document.onkeydown=function(e){document.getElementById('input').textContent='Key '+e.key;report('key',e.key);
 if(e.key==='PageDown'||e.key==='PageUp'){var box=document.getElementById('scrollbox');box.scrollTop+=(e.key==='PageDown'?80:-80);report('keyboard-js-scroll',box.scrollTop);}};
setTimeout(function(){var r=document.getElementById('button').getBoundingClientRect();report('geometry',JSON.stringify({x:r.left+r.width/2,y:r.top+r.height/2}));},2000);
for(var n=0;n<3;n++){(function(n){var i=new Image();images.push(i);i.onload=function(){report('asset-ok',n);};i.onerror=function(){report('asset-error',n);};i.src='/asset/'+n;})(n);}
var bad='''+json.dumps(bad)+''';
Object.keys(bad).forEach(function(name){var i=new Image();images.push(i);i.onload=function(){report('CERTIFICATE-ACCEPTED-INCORRECTLY',name);};i.onerror=function(){checks++;document.getElementById('tls').textContent='Rejected certificates '+checks+'/3';report('certificate-rejected',name);};i.src=bad[name];});
''').encode()
disk=out/'assets.img';create(disk,specs)
for server in servers:
    thread=threading.Thread(target=server.serve_forever,daemon=True);thread.start();threads.append(thread)
vm=None;visual=[];inputs=[];launches=[];samples=[]
resource_diagnostic=os.environ.get('MYOS_TLS_RESOURCE_DIAGNOSTIC')=='1'
resource_accounting=resource_diagnostic or os.environ.get('MYOS_TLS_RESOURCE_ACCOUNTING')=='1'
def observe_launch(launch):
    if resource_accounting:
        from cobalt_native_resource_inventory import capture
        capture(vm,out,'before-%d'%launch)
    launch_start=time.monotonic();request_start=len(records)
    offset=len(vm.log.read_text());type_text(vm,'elf pair-starboard cobalt_diag provider https://cobalt.example.com:'+str(servers[0].server_port)+'/\n')
    diagnostic=os.environ.get('MYOS_TLS_TASK_SNAPSHOT')=='1'
    deadline=time.monotonic()+(210 if diagnostic else 600)
    last=-10;visible=None;sent=False;snapshot=False;repeated=set();last_sample=-2;inode_captured=False
    pointer_requested=False;pointer_seen=-1;mouse_last=0;click_down=None;click_attempts=0
    from PIL import Image
    while time.monotonic()<deadline:
        elapsed=time.monotonic()-launch_start;content=vm.log.read_text()[offset:]
        if resource_diagnostic and elapsed-last_sample>=2:
            last_sample=elapsed
            registers=vm.command('human-monitor-command',{'command-line':'info registers'})
            match=re.search(r'RIP=([0-9a-fA-F]+)',registers)
            assert match,'QEMU register format changed'
            samples.append(dict(launch=launch,elapsed=elapsed,rip=int(match[1],16)))
        if 'PANIC' in content or content.endswith('myos> '):break
        if resource_diagnostic and not inode_captured and 'Creating shared memory in' in content and 'failed: Error 28' in content:
            capture(vm,out,'active-enospc-%d'%launch,include_heap=False)
            inode_captured=True
        if diagnostic and elapsed>=150 and not snapshot:
            from cobalt_task_snapshot import capture
            capture(vm,out);snapshot=True
            print('Paused task snapshot is diagnostic only, NEVER responsive/timing acceptance',flush=True)
        if 'DidFinishNavigation:' in content and 'net::ERR_' in content:
            print('Native HTTPS initial navigation failed; preserving evidence, not waiting for nonexistent DOM',flush=True);break
        if elapsed-last>=10:
            last=elapsed;frame=out/('launch-%d-frame-%03d.ppm'%(launch,elapsed))
            vm.command('screendump',{'filename':str(frame)})
            picture=Image.open(frame).convert('RGB')
            blue=sum(n for n,c in picture.getcolors(480000) if c==(216,240,255))
            visual.append(dict(launch=launch,elapsed=elapsed,blue_pixels=blue,frame=frame.name))
            if blue>=10000 and visible is None:visible=time.monotonic()
        geometries=[r for r in records[request_start:] if 'kind=geometry&' in r['path']]
        if visible is not None and time.monotonic()-visible>=5 and not sent and geometries:
            target=json.loads(urllib.parse.parse_qs(urllib.parse.urlsplit(geometries[-1]['path']).query)['value'][0])
            assert 0<=target['x']<800 and 0<=target['y']<600
            vm.key('a')
            sent=True;inputs.append(dict(launch=launch,elapsed=elapsed,key='a',click=False))
        # Only genuine DOM mousemove feedback supplies position. Never set
        # coordinates/callbacks in guest JS or infer delivery from QMP success.
        launch_records=records[request_start:]
        pointers=[r for r in launch_records if 'kind=pointer&' in r['path']]
        clicks=[r for r in launch_records if 'kind=click&' in r['path']]
        if sent and not pointer_requested:
            vm.command('input-send-event',{'events':[{'type':'rel','data':{'axis':'x','value':1}}]})
            pointer_requested=True;mouse_last=time.monotonic()
        if click_down is not None and time.monotonic()-click_down>=.2:
            vm.command('input-send-event',{'events':[{'type':'btn','data':{'down':False,'button':'left'}}]})
            inputs.append(dict(launch=launch,elapsed=elapsed,mouse='release'))
            click_down=None;mouse_last=time.monotonic()
        if sent and pointers and not clicks and click_down is None and time.monotonic()-mouse_last>=.3:
            pointer=json.loads(urllib.parse.parse_qs(urllib.parse.urlsplit(pointers[-1]['path']).query)['value'][0])
            if pointer['seq']!=pointer_seen:
                pointer_seen=pointer['seq'];dx=round(target['x']-pointer['x']);dy=round(target['y']-pointer['y'])
                if abs(dx)<=3 and abs(dy)<=3 and click_attempts<3:
                    vm.command('input-send-event',{'events':[{'type':'btn','data':{'down':True,'button':'left'}}]})
                    click_down=time.monotonic();click_attempts+=1
                    inputs.append(dict(launch=launch,elapsed=elapsed,mouse='press',pointer=pointer,target=target))
                else:
                    for axis,value in (('x',dx),('y',dy)):
                        if value:vm.command('input-send-event',{'events':[{'type':'rel','data':{'axis':axis,'value':max(-32,min(32,value))}}]})
                mouse_last=time.monotonic()
        if visible is not None and sent:
            for delay,keycode in ((40,'b'),(60,'pgdn'),(75,'c'),(90,'pgup')):
                if time.monotonic()-visible>=delay and keycode not in repeated:
                    vm.key(keycode);repeated.add(keycode)
                    inputs.append(dict(launch=launch,elapsed=elapsed,key=keycode,click=False))
        if visible is not None and time.monotonic()-visible>=(40 if resource_diagnostic else 120):break
        time.sleep(.1)
    vm.command('screendump',{'filename':str(out/('launch-%d-final.ppm'%launch))})
    if not vm.log.read_text().endswith('myos> '):
        vm.key('esc');end=time.monotonic()+20
        while time.monotonic()<end and not vm.log.read_text().endswith('myos> '):time.sleep(.1)
    tail=vm.log.read_text()[offset:]
    cleanup=[line for line in tail.splitlines() if 'pages reclaimed' in line or 'all threads stopped' in line]
    result=dict(launch=launch,elapsed=time.monotonic()-launch_start,
        request_start=request_start,request_end=len(records),cleanup=cleanup,
        shell_returned=vm.log.read_text().endswith('myos> '))
    launches.append(result)
    (out/'launches.json').write_text(json.dumps(launches,indent=2))
    print('LAUNCH_OBSERVATION',json.dumps(result),flush=True)
    assert result['shell_returned'],'close did not return to shell; cannot safely relaunch'
    if resource_accounting:
        capture(vm,out,'after-%d'%launch)
try:
    vm=VM('native-verified-tls',ram=512,nic=True,asset_disk=disk,cpu='max')
    vm.wait('myos> ');command(vm,'net up','Network: DHCP configured');command(vm,'mkdir /dev/shm','myos> ')
    for launch in range(1,1+int(os.environ.get('MYOS_TLS_LAUNCHES','1'))):
        observe_launch(launch)
finally:
    if vm:
        (out/'serial.txt').write_text(vm.log.read_text());vm.close()
    summary=dict(requests=records,peak_concurrent_assets=peak,visual=visual,inputs=inputs,launches=launches,
        diagnostic_only=resource_diagnostic,cpu_samples=samples,
        test_root_sha256=hashlib.sha256(ca_file.read_bytes()).hexdigest(),test_root_asset='ssl/certs/'+ca_hash+'.0')
    (out/'evidence.json').write_text(json.dumps(summary,indent=2))
    for server in servers:server.shutdown();server.server_close()
    for thread in threads:thread.join(timeout=2)
    print('EVIDENCE_BUNDLE',out,'requests',len(records),'peak_concurrent_assets',peak,flush=True)
    print('OBSERVATION ONLY: inspect actual native screenshot, TLS rejection errors, concurrency and cleanup; not full acceptance.',flush=True)
assert not any(r['server']!='valid' for r in records),'invalid certificate server received HTTP request'
assert not any('CERTIFICATE-ACCEPTED-INCORRECTLY' in r['path'] for r in records),'invalid certificate accepted'
