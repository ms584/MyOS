"""Read captured guest pixels/PCM and allocator counters; never decode media.
Validates one uninterrupted standard-media observation, not the whole phase.
"""
import argparse,hashlib,json,math,re,subprocess,urllib.parse,wave
from pathlib import Path
from PIL import Image
p=argparse.ArgumentParser(description=__doc__)
p.add_argument('bundle',type=Path);p.add_argument('--provider',type=Path,required=True)
p.add_argument('--renderer',type=Path,required=True);a=p.parse_args()
d=json.loads((a.bundle/'evidence.json').read_text());serial=(a.bundle/'serial.log').read_text()
digest=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
assert d['continuous_playback'] and not d['fault_mode'] and not d['diagnostic_trace']
assert digest(a.provider)==d['provider_sha256']
assert digest(a.bundle/'boot.img')==d['boot_sha256']
assert digest(a.bundle/'platform-runtime.mrt')==d['runtime_sha256']
assert 'USER FAULT' not in serial and 'PANIC' not in serial
metrics=[dict((k,int(v)) for k,v in re.findall(r'([\w-]+)=(\d+)',line))
 for line in serial.splitlines() if line.startswith('MEDIA native metrics ')]
assert len(metrics)==1,metrics
m=metrics[0]
assert m['decoded']==600 and m['dropped']==0 and m['corrupted']==0,m
assert m['sync-samples']>=450 and m['max-sync-us']<=100000,m
events=[]
for r in d['requests']:
 if r['path'].startswith('/evidence?'):
  q=urllib.parse.parse_qs(urllib.parse.urlsplit(r['path']).query)
  events.append(dict(elapsed=r['elapsed'],kind=q['kind'][0],value=q.get('value',[''])[0]))
assert not any(e['kind'] in ('error','fetch-error','append-error','sourcebuffer-error') for e in events)
playing=next(e['elapsed'] for e in events if e['kind']=='playing')
ended=next(e for e in events if e['kind']=='ended')
assert ended['value'].startswith('40 ') and ended['elapsed']-playing>=30,ended
visible=[]
for f in d['frames']:
 if playing+1<f['elapsed']<ended['elapsed']:
  im=Image.open(a.bundle/f['path']).convert('RGB')
  # Standard fixture binary cells in the observed 320x180 guest viewport.
  ident=sum(1<<b for b in range(10) if sum(im.getpixel((16+b*30+14,198)))>350)
  assert 0<=ident<600,(f,ident)
  visible.append(dict(elapsed=f['elapsed'],frame=ident,path=f['path'],sha256=digest(a.bundle/f['path'])))
assert len(visible)>=15
assert all(y['frame']>x['frame'] for x,y in zip(visible,visible[1:])),visible
span=(visible[-1]['frame']-visible[0]['frame'])/15
fps=(visible[-1]['frame']-visible[0]['frame'])/(visible[-1]['elapsed']-visible[0]['elapsed'])
assert span>=30 and 14<=fps<=16,(span,fps)
with wave.open(str(a.bundle/'audio.wav'),'rb') as w:
 assert (w.getnchannels(),w.getsampwidth(),w.getframerate())==(1,1,11025),w.getparams()
 data=w.readframes(w.getnframes());rate=w.getframerate();seconds=len(data)/rate
assert seconds>=30
tones=[]
for second,expected in ((5,440),(15,660),(25,880),(35,550)):
 segment=[x-128 for x in data[second*rate:(second+1)*rate]]
 rms=math.sqrt(sum(x*x for x in segment)/len(segment))
 crossings=sum(segment[i-1]<0<=segment[i] for i in range(1,len(segment)))
 assert rms>20 and abs(crossings-expected)<=5,(second,rms,crossings)
 tones.append(dict(second=second,rms_u8=rms,positive_crossings=crossings,expected_hz=expected))
gaps=[];start=0
for i in range(1,len(data)+1):
 if i==len(data) or data[i]!=data[start]:
  if i-start>=100 and abs(data[start]-128)<=2:
   gaps.append(dict(second=start/rate,duration=(i-start)/rate,value=data[start]))
  start=i
assert not any(g['second']>=1 and g['second']<39 for g in gaps),gaps
baseline=d['ram_baselines'][0];rows=[r for r in d['ram_samples'] if 'pmm_history_complete' in r]
assert rows and all(r['pmm_history_complete'] for r in rows)
assert rows[-1]['label']=='after-process-exit',rows[-1]
minimum=min(r['pmm_interval_min_free_pages'] for r in rows)
physical=(baseline['free_pages']-minimum)*4096
heap=max(r['heap_runtime_peak_bytes'] for r in rows)-baseline['heap_live_bytes']
def mapped_bytes(path):
 pages=set()
 for line in subprocess.check_output(['readelf','-lW',str(path)],text=True).splitlines():
  f=line.split()
  if f and f[0]=='LOAD':
   address=int(f[2],16);size=int(f[5],16)
   pages.update(range(address//4096,(address+size+4095)//4096))
 return len(pages)*4096
provider_delta=mapped_bytes(a.provider)-mapped_bytes(a.renderer)
assert provider_delta>=0
total=physical+max(0,heap)+provider_delta
assert total<=32*1024*1024,(physical,heap,provider_delta,total)
resources=re.findall(r'ELF resident: free-before=(\d+) free-after=(\d+) shm-before=(\d+) shm-after=(\d+) aliases-before=(\d+) aliases-after=(\d+)',serial)
assert len(resources)==1 and all(x==y for x,y in zip(resources[0][::2],resources[0][1::2])),resources
assert len(d['shell_checks'])==1 and 'Free physical pages:' in d['shell_checks'][0]['output']
report=dict(continuous_session_passed=True,whole_phase_passed=False,
 provider_sha256=d['provider_sha256'],boot_sha256=d['boot_sha256'],runtime_sha256=d['runtime_sha256'],
 fixture_sha256=d['fixture_sha256'],metrics=m,visible_frames=visible,visible_motion_span_seconds=span,
 measured_visible_fps=fps,audio_sha256=digest(a.bundle/'audio.wav'),audio_seconds=seconds,
 audio_tones=tones,pcm_constant_gaps=gaps,ram=dict(physical_peak_increment_bytes=physical,
 conservative_heap_increment_bytes=max(0,heap),preloaded_media_provider_delta_bytes=provider_delta,
 total_bound_bytes=total,budget_bytes=33554432,history_rows=len(rows),
 max_history_gap_ticks=max(r['pmm_history_coverage_ticks'] for r in rows)),
 resource_rows=resources,shell_checks=d['shell_checks'])
(a.bundle/'continuous-analysis.json').write_text(json.dumps(report,indent=2)+'\n')
print('PASS continuous native session',a.bundle,'sync-us',m['max-sync-us'],'RAM-bound',total,'visible-fps',fps,flush=True)
