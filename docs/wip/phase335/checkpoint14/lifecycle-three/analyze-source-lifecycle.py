"""Scoped lifecycle audit of native captured output; never Phase33.5 acceptance."""
from pathlib import Path
import json,re,sys,urllib.parse,hashlib,subprocess
from PIL import Image
b=Path(sys.argv[1]);d=json.loads((b/'evidence.json').read_text());serial=(b/'serial.log').read_text()
assert d['lifecycle_scenario'] and not d['diagnostic_trace'] and not d['fault_mode']
assert 'USER FAULT' not in serial and 'PANIC' not in serial
resident=re.findall(r'ELF resident: free-before=(\d+) free-after=(\d+) shm-before=(\d+) shm-after=(\d+) aliases-before=(\d+) aliases-after=(\d+)',serial)
assert len(resident)==3 and all(all(a==c for a,c in zip(r[::2],r[1::2])) for r in resident),resident
heaps=[tuple(map(int,r)) for r in re.findall(r'ELF pair live: pages reclaimed; heap-before=(\d+) heap-after=(\d+)',serial)]
# Original documented warm pattern may add4096B on the second launch.
# Require the third boundary to be stable; retain all earlier increments.
assert len(heaps)==3 and heaps[2][0]==heaps[2][1],heaps
metrics=[dict((k,int(v)) for k,v in re.findall(r'([\w-]+)=(-?\d+)',line)) for line in serial.splitlines() if line.startswith('MEDIA native metrics ')]
assert len(metrics)==6 and all(m['corrupted']==0 and m['max-sync-us']<=100000 for m in metrics),metrics
assert len(d['shell_checks'])==3 and all('Free physical pages:' in r['output'] for r in d['shell_checks'])
events=[]
for r in d['requests']:
 if r['path'].startswith('/evidence?'):
  q=urllib.parse.parse_qs(urllib.parse.urlsplit(r['path']).query);events.append(dict(elapsed=r['elapsed'],kind=q['kind'][0],value=q.get('value',[''])[0]))
assert not any(e['kind'] in ('error','fetch-error','append-error','sourcebuffer-error') for e in events)
def frame_id(f):
 im=Image.open(b/f['path']).convert('RGB')
 return sum(1<<bit for bit in range(10) if sum(im.getpixel((16+bit*30+14,198)))>350)
def mapped_bytes(path):
 pages=set()
 for line in subprocess.check_output(['readelf','-lW',str(path)],text=True).splitlines():
  f=line.split()
  if f and f[0]=='LOAD':
   address=int(f[2],16);size=int(f[5],16);pages.update(range(address//4096,(address+size+4095)//4096))
 return len(pages)*4096
root=Path('/home/xytron404/myos')
provider=root/'build/media-evaluation/provider-b6mzu_qt/provider.so';renderer=root/'build/renderer-provider-evaluation/provider-84sokh6g/provider.so'
assert hashlib.sha256(provider.read_bytes()).hexdigest()==d['provider_sha256']
provider_delta=mapped_bytes(provider)-mapped_bytes(renderer);assert provider_delta>=0
rows=[]
for n in range(3):
 fs=[f for f in d['frames'] if f['launch']==n];inputs=[i for i in d['inputs'] if i['launch']==n]
 assert fs and all(any(i['key']==key for i in inputs) for key in ('b','p','r','s','n','x')),inputs
 lower=min(f['elapsed'] for f in fs);following=[f['elapsed'] for f in d['frames'] if f['launch']==n+1];upper=min(following) if following else d['elapsed']+1
 es=[e for e in events if lower<=e['elapsed']<upper]
 playing=[e for e in es if e['kind']=='playing'];ended=[e for e in es if e['kind']=='ended']
 assert len(playing)>=2 and ended and ended[0]['value'].startswith('40 '),(n,playing,ended)
 times={i['key']:i['elapsed'] for i in inputs}
 pick=lambda t:min(fs,key=lambda f:abs(f['elapsed']-t))
 before=pick(times['p']-1);resume=pick(times['r']+2);seek=pick(times['s']+2);restart=pick(times['n']+5)
 ids=[frame_id(f) for f in (before,resume,seek,restart)]
 assert all(0<=i<600 for i in ids) and ids[1]>ids[0] and ids[2]>ids[1] and ids[3]<ids[2],(n,ids)
 settled=[f for f in fs if times['p']+.5<=f['elapsed']<=times['r']-.2]
 pause=dict(samples=len(settled),pair_verified=False)
 if len(settled)>=2:
  images=[Image.open(b/f['path']).convert('RGB').crop((16,182,336,362)).tobytes() for f in settled]
  assert len(set(images))==1 and len({f['pcm_file_bytes'] for f in settled})==1,(n,settled)
  pause.update(pair_verified=True,observed_seconds=settled[-1]['elapsed']-settled[0]['elapsed'])
 hardware=[r for r in d['ram_samples'] if r['launch']==n]
 held=[r for r in hardware if times['p']+.5<=r['elapsed']<=times['r']-.2]
 assert len(held)>=2 and len({(r['irq_count'],r['consumed_frames']) for r in held})==1,(n,held)
 resumed=[r for r in hardware if times['r']+1<=r['elapsed']<times['s']-.2]
 assert resumed and max(r['irq_count'] for r in resumed)>held[-1]['irq_count'],(n,resumed)
 stopped=[r for r in hardware if times['x']+.5<=r['elapsed']]
 assert len(stopped)>=2 and len({r['irq_count'] for r in stopped})==1 and all(r['consumed_frames']==0 for r in stopped),(n,stopped)
 black=[f for f in fs if times['x']+.5<=f['elapsed']]
 assert black and all(not any(Image.open(b/f['path']).convert('RGB').crop((16,182,336,362)).tobytes()) for f in black),(n,black)
 pause.update(hardware_samples=held,hardware_pair_verified=True)
 history=[r for r in d['ram_samples'] if r['launch']==n and 'pmm_history_complete' in r]
 assert history and all(r['pmm_history_complete'] for r in history) and history[-1]['label']=='after-process-exit',history
 baseline=next(r for r in d['ram_baselines'] if r['launch']==n)
 physical=(baseline['free_pages']-min(r['pmm_interval_min_free_pages'] for r in history))*4096
 heap=max(0,max(r['heap_runtime_peak_bytes'] for r in history)-baseline['heap_live_bytes'])
 ram=dict(physical_peak_increment_bytes=physical,conservative_heap_increment_bytes=heap,preloaded_media_provider_delta_bytes=provider_delta,total_bound_bytes=physical+heap+provider_delta,budget_bytes=33554432,max_history_gap_ticks=max(r['pmm_history_coverage_ticks'] for r in history))
 assert ram['total_bound_bytes']<=ram['budget_bytes'],(n,ram)
 rows.append(dict(launch=n,ram=ram,inputs=inputs,representative_frames=[dict(frame=f,id=i) for f,i in zip((before,resume,seek,restart),ids)],pause_observation=pause,stopped_hardware=stopped,black_stop_frames=black,events=es,history_complete=True,history_rows=len(history)))
report=dict(scoped_lifecycle_controls_resources_passed=True,whole_phase_passed=False,music_listened=False,owner_acceptance=False,prior_continuous_failures_resolved=False,provider_sha256=d['provider_sha256'],resident=resident,heap_boundaries=heaps,metrics=metrics,launches=rows)
(b/'lifecycle-analysis.json').write_text(json.dumps(report,indent=2)+'\n')
print('PASS scoped3-launch lifecycle/captured frame transitions/EOS/restart/shell/residents/warm heap; not full audio or phase acceptance',b)
