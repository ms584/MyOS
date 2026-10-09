"""Audit pixels/native PCM/counters from a MyOS public observation.
Never fetch or decode media. This records a scoped observation, never whole-phase acceptance.
Human inspection of the actual movie/Stats frames remains required.
"""
import argparse,hashlib,json,math,re,wave
from pathlib import Path
from PIL import Image,ImageChops
ap=argparse.ArgumentParser(description=__doc__)
ap.add_argument('bundle',type=Path)
ap.add_argument('--provider',type=Path,required=True)
ap.add_argument('--motion',action='append',required=True,help='launch:start:end seconds')
ap.add_argument('--pause',action='append',required=True,help='launch:start:end seconds after pause before resume')
a=ap.parse_args();d=json.loads((a.bundle/'evidence.json').read_text())
serial=(a.bundle/'serial.log').read_text()
digest=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
assert digest(a.provider/'provider.so')==d['provider_sha256']
assert d['normal_identity']==d['normal_identity_after']
assert not d['host_fetch'] and not d['fixture_ca_added']
assert not any(x in serial for x in ['USER FAULT','USERFAULT','PANIC'])
roi=(40,80,740,350)
def spec(s):
 n,start,end=s.split(':');return int(n),float(start),float(end)
def frames(s):
 n,start,end=spec(s);row=next(r for r in d['launches'] if r['launch']==n)
 return n,[f for f in row['frames'] if start<=f['elapsed']<=end]
def image_delta(x,y):
 im1=Image.open(a.bundle/x['path']).convert('RGB').crop(roi)
 im2=Image.open(a.bundle/y['path']).convert('RGB').crop(roi)
 diff=ImageChops.difference(im1,im2)
 pixels=diff.get_flattened_data() if hasattr(diff,'get_flattened_data') else diff.getdata()
 return sum(pixel!=(0,0,0) for pixel in pixels)
motion=[]
for s in a.motion:
 n,fs=frames(s);assert len(fs)>=7
 changes=[dict(before=x['path'],after=y['path'],changed_pixels=image_delta(x,y)) for x,y in zip(fs,fs[1:])]
 span=fs[-1]['elapsed']-fs[0]['elapsed']
 assert span>=30 and all(r['changed_pixels']>500 for r in changes)
 motion.append(dict(launch=n,observed_span_seconds=span,roi=roi,changes=changes))
pauses=[]
for s in a.pause:
 n,fs=frames(s);assert len(fs)>=2 and fs[-1]['elapsed']-fs[0]['elapsed']>=8
 changes=image_delta(fs[0],fs[-1]);assert changes==0
 pauses.append(dict(launch=n,before=fs[0]['path'],after=fs[-1]['path'],elapsed_span=fs[-1]['elapsed']-fs[0]['elapsed'],roi=roi,changed_pixels=changes))
groups=[];group=None
for line in serial.splitlines():
 if line.startswith('MEDIA stream '):
  group=dict(format=line,source=None,metrics=[],final_metrics=[],hardware=[],first_video=[],rebuffer_events=[])
  groups.append(group)
 elif group is not None:
  if line.startswith('MEDIA decoded '):group['source']=line
  elif line.startswith('MEDIA metrics '):
   group['metrics'].append(dict((k,int(v)) for k,v in re.findall(r'([\w-]+)=(-?\d+)',line)))
  elif line.startswith('MEDIA native metrics '):
   group['final_metrics'].append(dict((k,int(v)) for k,v in re.findall(r'([\w-]+)=(-?\d+)',line)))
  elif line.startswith('MEDIA hardware-clock '):
   group['hardware'].append(dict((k,int(v)) for k,v in re.findall(r'([\w-]+)=(-?\d+)',line)))
  elif line.startswith('MEDIA first-video '):
   group['first_video'].append(dict((k,int(v)) for k,v in re.findall(r'([\w-]+)=(-?\d+)',line)))
  elif line.startswith('MEDIA rebuffer '):group['rebuffer_events'].append(line)
  elif line.startswith('MEDIA sync-outlier '):raise AssertionError('Actual native sync outlier retained; inspect private native log')
assert groups and all(g['metrics'] and g['hardware'] for g in groups)
held=[];peak=0
for g in groups:
 g['max_sync_us']=max(m['max-sync-us'] for m in g['metrics']+g['final_metrics'])
 assert g['max_sync_us']<=100000
 assert all(h['failures']==0 for h in g['hardware'])
 assert g['first_video'] and all(v['consumed']==0 for v in g['first_video'])
 assert all(m['corrupted']==0 for m in g['metrics'])
 peak=max(peak,max(m['ram-query-bytes'] for m in g['metrics']))
 zero=[m for m in g['metrics'] if m['rate']==0]
 for x,y in zip(zero,zero[1:]):
  if y['mono-us']-x['mono-us']>=1000000 and x['position-us']==y['position-us'] and x['consumed']==y['consumed'] and x['presented']==y['presented']:
   held.append(dict(position_us=x['position-us'],consumed=x['consumed'],presented=x['presented'],held_us=y['mono-us']-x['mono-us']))
assert len(held)>=len(a.pause)
with wave.open(str(a.bundle/'audio.wav'),'rb') as w:
 assert (w.getframerate(),w.getnchannels(),w.getsampwidth())==(11025,1,1)
 pcm=w.readframes(w.getnframes())
rms=[]
for start in range(0,len(pcm)-11025+1,11025):
 xs=pcm[start:start+11025];rms.append(math.sqrt(sum((x-128)**2 for x in xs)/11025))
assert sum(x>1 for x in rms)>=30
res=re.findall(r'ELF resident: free-before=(\d+) free-after=(\d+) shm-before=(\d+) shm-after=(\d+) aliases-before=(\d+) aliases-after=(\d+)',serial)
assert len(res)==len(d['launches']) and all(all(x==y for x,y in zip(row[::2],row[1::2])) for row in res)
for row in d['launches']:
 assert row['shell_returned'] and row.get('desktop_after_close')
 counts=[re.search(r'Free physical pages: (\d+)',row[k]).group(1) for k in ['before','after','after_desktop']]
 assert len(set(counts))==1
report=dict(phase_passed=False,scoped_pixel_pcm_clock_observation_pass=True,commit=d['commit'],date=d['date'],
 provider_sha256=d['provider_sha256'],source_sha256=json.loads((a.provider/'report.json').read_text())['player_source_sha256'],
 motion=motion,pause_movie_roi=pauses,hardware_held_samples=held,native_player_groups=groups,
 native_pcm_seconds=len(pcm)/11025,native_pcm_audible_second_bins=sum(x>1 for x in rms),native_pcm_sha256=digest(a.bundle/'audio.wav'),
 native_pcm_rms_by_second=rms,owned_mapping_peak_bytes=peak,actual_qemu_ram_mib=d['memory_mib'],
 resident_exact=True,normal_identity_exact=True,desktop_and_shell_all_closes=True,
 signed_url_actual_expiry_proved=False,
 scope='Pixels are native display capture; PCM is same-session native SB16 active sample time, not wall-time. Native rate0 PCM_STATUS hold plus FLUSH proves silence during pause. Motion/Stats/control/recovery frames and input timeline need human audit; this script never closes33.4/33.5/33/Rickroll.')
(a.bundle/'native-observation-analysis.json').write_text(json.dumps(report,indent=2)+'\n')
print('PASS scoped native pixel/PCM/clock observation; whole phase incomplete',a.bundle)
print('actual sync-us',[g['max_sync_us'] for g in groups],'owned mapping peak',peak,'PCM seconds',len(pcm)/11025)
