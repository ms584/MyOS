"""Audit recorded native Rickroll pixels/PCM/hardware/resources. Never whole-phase acceptance.
Song identity/listening, live controls and user signoff remain interactive gates.
"""
import argparse,json,re,math,wave,hashlib
from pathlib import Path
from PIL import Image,ImageChops
p=argparse.ArgumentParser(description=__doc__);p.add_argument('bundle',type=Path);a=p.parse_args();b=a.bundle
r=json.loads((b/'evidence.json').read_text());s=(b/'serial.log').read_text();roi=(40,80,740,350)
def diff(x,y):
    d=ImageChops.difference(Image.open(b/x).convert('RGB').crop(roi),Image.open(b/y).convert('RGB').crop(roi))
    return sum(v!=(0,0,0) for v in d.getdata())
launches=[]
for row in r['launches']:
    fs=row['frames'];pick=lambda t:min(fs,key=lambda f:abs(f['elapsed']-t))
    movie=[pick(t) for t in range(155,201,5)];changes=[diff(x['path'],y['path']) for x,y in zip(movie,movie[1:])]
    x,y=pick(209),pick(218);pause=diff(x['path'],y['path'])
    controls=[]
    for name,before,after in [('resume',218,228),('forward',239,249),('backward',264,279)]:
        x,y=pick(before),pick(after);controls.append(dict(control=name,before=x,after=y,changed_movie_pixels=diff(x['path'],y['path'])))
    timeline=row['recording_timeline'];closes=[i['elapsed'] for i in row['inputs'] if i['action']['id'].startswith('player-close')];close_time=closes[-1] if closes else None;audio_at=lambda t:min(timeline,key=lambda f:abs(f['elapsed']-t))['wav_bytes']
    launches.append(dict(pause_captured_pcm_byte_delta=audio_at(218)-audio_at(209),resume_captured_pcm_byte_delta=audio_at(228)-audio_at(218),close_captured_pcm_byte_delta=audio_at(close_time+17)-audio_at(close_time+5) if close_time is not None and row['elapsed']>close_time+17 else None,inode_before=row.get('inode_before'),inode_after=row.get('inode_after'),launch=row['launch'],movie_span_seconds=movie[-1]['elapsed']-movie[0]['elapsed'],movie_changes=changes,motion_observed=all(v>500 for v in changes),pause_movie_pixels=pause,pause_span_seconds=pick(218)['elapsed']-pick(209)['elapsed'],controls=controls,shell_returned=row['shell_returned'],desktop=row.get('desktop_after_close'),resources=row.get('after'),inputs=row['inputs']))
metrics=[dict((k,int(v)) for k,v in re.findall(r'([\w-]+)=(-?\d+)',line)) for line in s.splitlines() if line.startswith(('MEDIA metrics ','MEDIA native metrics '))]
hw=[dict((k,int(v)) for k,v in re.findall(r'([\w-]+)=(-?\d+)',line)) for line in s.splitlines() if line.startswith('MEDIA hardware-clock ')]
held=[]
zero=[m for m in metrics if m.get('rate')==0]
for x,y in zip(zero,zero[1:]):
    if all(x[k]==y[k] for k in ('position-us','consumed','presented')) and y['mono-us']-x['mono-us']>=1000000:
        held.append(dict(position_us=x['position-us'],consumed=x['consumed'],presented=x['presented'],held_us=y['mono-us']-x['mono-us']))
with wave.open(str(b/'audio.wav'),'rb') as w:
    assert (w.getframerate(),w.getsampwidth(),w.getnchannels())==(11025,1,1);pcm=w.readframes(w.getnframes())
rms=[math.sqrt(sum((v-128)**2 for v in pcm[t:t+11025])/11025) for t in range(0,len(pcm)-11024,11025)]
resources=re.findall(r'ELF resident: free-before=(\d+) free-after=(\d+) shm-before=(\d+) shm-after=(\d+) aliases-before=(\d+) aliases-after=(\d+)',s)
exact=len(resources)==len(launches) and all(all(x==y for x,y in zip(v[::2],v[1::2])) for v in resources)
out=dict(phase_passed=False,user_acceptance_received=False,song_listening_verified=False,commit=r['commit'],date=r['date'],urls=r['urls'],launches=launches,native_pcm_seconds=len(pcm)/11025,audible_rms_second_bins=sum(v>1 for v in rms),pcm_sha256=hashlib.sha256((b/'audio.wav').read_bytes()).hexdigest(),hardware_held=held,hardware_query_failures=max((m['failures'] for m in hw),default=None),max_sync_us=max((m['max-sync-us'] for m in metrics),default=None),owned_mapping_peak_bytes=max((m['ram-query-bytes'] for m in metrics if 'ram-query-bytes' in m),default=None),sampled_pmm=r.get('pmm'),metrics=metrics,seek_tickets=re.findall(r'MEDIA seek ticket=(\d+) target-us=(\d+)',s),first_video=re.findall(r'MEDIA first-video ticket=(\d+) pts=(\d+) clock=(\d+) consumed=(\d+)',s),decoded_source_lines=[v for v in s.splitlines() if v.startswith('MEDIA decoded ')],stream_formats=[v for v in s.splitlines() if v.startswith('MEDIA stream ')],sync_outliers=[v for v in s.splitlines() if v.startswith('MEDIA sync-outlier ')],resources_exact=exact,normal_identity_exact=r['normal_identity']==r['normal_identity_after'],faults=[v for v in s.splitlines() if any(t in v for t in ['USER FAULT','USERFAULT','PANIC'])],scope='Native PNG/PCM/hardware/counter observations; metrics do not identify music by listening, inspect every stale-AV transition, establish exact RSS/high-water RAM, or grant interactive owner acceptance. AVI timing is approximate. Failed runs remain failed.')
(b/'phase335-analysis.json').write_text(json.dumps(out,indent=2)+'\n')
print('OBSERVATION ONLY',b,'motion',[(x['movie_span_seconds'],x['motion_observed']) for x in launches],'pause',[x['pause_movie_pixels'] for x in launches],'PCM',out['native_pcm_seconds'],'sync',out['max_sync_us'],'exact',exact)
