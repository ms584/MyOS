"""Audit finite repeated native online opens/closes; never owner/sound acceptance."""
import argparse,json,re,wave
from pathlib import Path
from PIL import Image,ImageChops
p=argparse.ArgumentParser(description=__doc__);p.add_argument('bundle',type=Path);a=p.parse_args();b=a.bundle
d=json.loads((b/'evidence.json').read_text());s=(b/'serial.log').read_text()
resident=re.findall(r'ELF resident: free-before=(\d+) free-after=(\d+) shm-before=(\d+) shm-after=(\d+) aliases-before=(\d+) aliases-after=(\d+)',s)
heaps=[tuple(map(int,x)) for x in re.findall(r'ELF pair live: pages reclaimed; heap-before=(\d+) heap-after=(\d+)',s)]
rows=[]
for n,r in enumerate(d['launches']):
 pick=lambda t:min(r['frames'],key=lambda f:abs(f['elapsed']-t))
 fs=[pick(t) for t in range(111,162,5)];changes=[]
 for x,y in zip(fs,fs[1:]):
  delta=ImageChops.difference(Image.open(b/x['path']).convert('RGB').crop((120,80,520,250)),Image.open(b/y['path']).convert('RGB').crop((120,80,520,250)))
  changes.append(sum(v!=(0,0,0) for v in delta.getdata()))
 timeline=r['recording_timeline'];audio=lambda t:min(timeline,key=lambda x:abs(x['elapsed']-t))['wav_bytes']
 before=r['inode_before'];after=r['inode_after'];heap=heaps[n] if n<len(heaps) else None
 rows.append(dict(launch=r['launch'],movie_span_seconds=fs[-1]['elapsed']-fs[0]['elapsed'],movie_changed_pixels=changes,motion_observed=all(v>500 for v in changes),movie_window_pcm_bytes=audio(161)-audio(111),post_close_pcm_bytes=audio(176)-audio(170),heap_before=heap[0] if heap else None,heap_after=heap[1] if heap else None,linked_capacity_before=before['linked_file_buffer_capacity'],linked_capacity_after=after['linked_file_buffer_capacity'],unlinked_capacity_after=after['unlinked_file_buffer_capacity'],non_file_heap_before=heap[0]-before['linked_file_buffer_capacity'] if heap else None,non_file_heap_after=heap[1]-after['linked_file_buffer_capacity'] if heap else None,live_nodes_after=len(after['nodes']),normal_nodes_after=sum(x['index']<64 for x in after['nodes']),referenced_file_nodes_after=sum(x['references']>0 for x in after['nodes'] if x['allocated_file_data'] and not x['shared']),desktop=r['desktop_after_close'],shell_returned=r['shell_returned'],inputs=r['inputs']))
metrics=[dict((k,int(v)) for k,v in re.findall(r'([\w-]+)=(-?\d+)',x)) for x in s.splitlines() if x.startswith('MEDIA native metrics ')]
with wave.open(str(b/'audio.wav'),'rb') as w:pcm_seconds=w.getnframes()/w.getframerate()
out=dict(phase_passed=False,user_acceptance_received=False,commit=d['commit'],date=d['date'],provider_sha256=d['provider_sha256'],normal_identity_exact=d['normal_identity']==d['normal_identity_after'],launches=rows,resident_resources_exact=len(resident)==len(rows) and all(all(x==y for x,y in zip(v[::2],v[1::2])) for v in resident),native_metrics=metrics,max_sync_us=max((x['max-sync-us'] for x in metrics),default=None),sync_outliers=[x for x in s.splitlines() if x.startswith('MEDIA sync-outlier ')],faults=[x for x in s.splitlines() if any(t in x for t in ['USER FAULT','USERFAULT','USER SIGNAL','PANIC',':FATAL:'])],pcm_seconds=pcm_seconds,pmm=d.get('pmm'),scope='Finite repeated-open observations, actual changing movie region/native PCM and cleanup only. Movie identity requires inspecting selected full frames. Non-file heap subtracts linked file buffer capacity; allocator padding may differ. Cache retention is not zero heap, unlimited stability or proof of no silent cache I/O failure. Sampled PMM includes reserved heap backing; old observer caveat claiming otherwise was incorrect. Song listening/owner acceptance remain absent.')
(b/'phase335-stability-analysis.json').write_text(json.dumps(out,indent=2)+'\n')
print('FINITE OBSERVATION',b,'nodes/capacity',[(x['normal_nodes_after'],x['linked_capacity_after'],x['non_file_heap_after']) for x in rows],'sync',out['max_sync_us'],'exact',out['resident_resources_exact'])
