"""Attach to a visible normal MyOS VM and record native pixels/SB16 PCM.
Never fetches media, decodes YouTube, starts a VM or records phase acceptance.
All original PNG/WAV/timestamps remain authoritative; optional controls are
posted native key events, not proof that the player obeyed them.
"""
import argparse,datetime,hashlib,json,socket,time,tempfile,subprocess
from pathlib import Path
from PIL import Image
p=argparse.ArgumentParser(description=__doc__)
p.add_argument('session',type=Path);p.add_argument('--seconds',type=int,default=90)
p.add_argument('--ram-history',action='store_true',help='Sample existing native PMM history through this QMP client; brief pauses are disclosed')
p.add_argument('--key-at',action='append',default=[],help='Native control time:key; keys k,j,l only, e.g.35:k')
a=p.parse_args();root=Path.cwd().resolve();session=a.session.resolve()
assert session.is_relative_to(root/'build') and 30<=a.seconds<=180
assert subprocess.run(['pgrep','-f','python3 tests/test_system.py'],stdout=subprocess.DEVNULL).returncode!=0,'Full regression still live'
metadata=json.loads((session/'qemu-command.json').read_text())
bundle=Path(metadata['source_bundle']).resolve();frozen=metadata['frozen_inputs']
sha=lambda path:hashlib.sha256(path.read_bytes()).hexdigest()
assert all(sha(bundle/name)==value for name,value in frozen.items()),'Frozen normal inputs changed'
normal=json.loads((bundle/'evidence.json').read_text())['normal_identity']
assert all(sha(root/'build'/name)==value for name,value in normal.items()),'Current normal five do not match this session'
assert any(arg.startswith('pa,id=myos-audio') for arg in metadata['command']),'Requires live speaker output, not silent WAV-only playback'
assert not metadata.get('boot_check') and metadata['command'][metadata['command'].index('-display')+1]!='none','Requires a visible normal session'
keys=[]
for item in a.key_at:
 at,key=item.split(':');at=float(at)
 assert key in ('k','j','l') and 0<at<a.seconds
 keys.append((at,key))
keys.sort();out=Path(tempfile.mkdtemp(prefix='native-recording-',dir=session));wav=out/'audio.wav'
s=socket.socket(socket.AF_UNIX);s.settimeout(8);s.connect(str(session/'manual.sock'));f=s.makefile('rwb',buffering=0);f.readline()
def q(name,args=None):
 msg={'execute':name}
 if args is not None:msg['arguments']=args
 f.write((json.dumps(msg)+'\n').encode())
 while True:
  line=f.readline()
  if not line:raise RuntimeError('QMP disconnected')
  reply=json.loads(line)
  if 'error' in reply:raise RuntimeError(reply['error'])
  if 'return' in reply:return reply['return']
def hmp(command):return q('human-monitor-command',{'command-line':command})
q('qmp_capabilities')
observer=None
if a.ram_history:
 import sys
 sys.path.insert(0,str(root/'tests'));sys.path.insert(0,str(root/'docs/wip/phase335/checkpoint2'))
 from cobalt_pmm_history_observer import HistoryObserver
 class Attached:
  command=staticmethod(q)
  @staticmethod
  def memory(address,size):
   tmp=out/'ram-read.bin';q('pmemsave',{'val':address,'size':size,'filename':str(tmp)})
   data=tmp.read_bytes();tmp.unlink();assert len(data)==size;return data
 observer=HistoryObserver(Attached(),memory_mib=1024)
 observer.capture(1,0,'recording baseline; excludes earlier load/playback')
capture_inventory=hmp('info capture').strip()
deprecation="warning: 'info capture' is deprecated since v10.2, to be removed"
active_capture=chr(10).join(line for line in capture_inventory.splitlines() if line.strip()!=deprecation).strip()
assert not active_capture,'Audio capture inventory is not empty: '+repr(active_capture)
assert q('query-status')['running']
frames=[];timeline=[];inputs=[];capture_started=False;complete=False;last_digest=None;last_path=None
started=time.monotonic();began=datetime.datetime.now().astimezone().isoformat()
try:
 response=hmp(f'wavcapture "{wav}" myos-audio 11025 8 1');capture_started=True
 assert 'error' not in response.lower() and 'failed' not in response.lower(),response
 started=time.monotonic();next_frame=0;sent=set()
 while time.monotonic()-started<a.seconds:
  elapsed=time.monotonic()-started
  if elapsed>=next_frame:
   before=time.monotonic();ppm=out/'current.ppm';q('screendump',{'filename':str(ppm)});raw=ppm.read_bytes();digest=hashlib.sha256(raw).hexdigest()
   if digest!=last_digest:
    last_path=f'frame-{len(frames):04d}.png';Image.open(ppm).save(out/last_path);last_digest=digest
   ppm.unlink();
   if observer:observer.capture(1,time.monotonic()-started)
   frames.append({'elapsed':time.monotonic()-started,'path':last_path,'pixel_sha256':digest,'capture_seconds':time.monotonic()-before});next_frame=elapsed+1
  timeline.append({'elapsed':time.monotonic()-started,'wav_bytes':wav.stat().st_size if wav.exists() else 0})
  for index,(at,key) in enumerate(keys):
   if elapsed>=at and index not in sent:
    for down in (True,False):
     q('input-send-event',{'events':[{'type':'key','data':{'down':down,'key':{'type':'qcode','data':key}}}]});time.sleep(.04)
    inputs.append({'key':key,'elapsed':elapsed,'qmp_posted':True,'player_result':'requires actual pixel/audio/control audit'});sent.add(index)
  time.sleep(.05)
 complete=True
finally:
 cleanup_error=None
 if capture_started:
  try:hmp('stopcapture 0')
  except Exception as error:cleanup_error=str(error);complete=False
 timeline.append({'elapsed':time.monotonic()-started,'wav_bytes':wav.stat().st_size if wav.exists() else 0})
 evidence={'phase_passed':False,'user_acceptance_received':False,'capture_completed':complete,'cleanup_error':cleanup_error,'expected_url':'https://www.youtube.com/watch?v=dQw4w9WgXcQ','expected_url_verified_by_capture':False,'date':began,'source_session':str(session),'normal_identity':normal,'qemu':metadata['command'],'fps':1,'capture_only':'Real native QEMU pixels and guest SB16 PCM mirrored from live PulseAudio device. No host media fetching or codec decoding.','launches':[{'launch':1,'elapsed':time.monotonic()-started,'frames':frames,'recording_timeline':timeline,'inputs':inputs}]}
 if observer:
  evidence['ram_history']=observer.report()
  evidence['ram_history']['recording_scope_only']=True
 (out/'evidence.json').write_text(json.dumps(evidence,indent=2)+'\n');f.close();s.close()
 print('Native recording:',out,flush=True)
 print('No phase acceptance recorded. Review actual movie ROI, music, controls and native clock logs.',flush=True)
