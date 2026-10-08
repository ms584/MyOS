from pathlib import Path
import json,re,sys
root=Path('/home/xytron404/cobalt-closed-imports-xnVZeH')
if len(sys.argv)>1:bundle=root/'build'/sys.argv[1]
else:
 matches=re.findall(r'/build/(native-verified-tls-[a-z0-9_]+)/assets.img',
     (root/'cobalt-cpu-accounting.log').read_text())
 assert matches,'No actual QEMU bundle in CPU job log'
 bundle=root/'build'/matches[-1]
data=json.loads((bundle/'cpu-accounting.json').read_text())
print('BUNDLE',bundle,'SAMPLES',len(data),'PIT100Hz guest charges, NOT host CPU percentage')
print('Reads non-atomic; no paused-VM timing claim')
blocked_coverage=all(sample.get('blocked_task_coverage',False) for sample in data)
if not blocked_coverage:print('LIMITATION: old observer omitted !alive/blocked tasks; zero blocked counts INVALID, CPU counters remain valid')
previous={};intervals=[];totals={}
for sample in data:
 for process in sample['processes']:
  pid=process['pid'];before=previous.get(pid);previous[pid]=(sample,process)
  if not before:continue
  old_sample,old=before
  ticks=sample['guest_tick_before']-old_sample['guest_tick_after']
  charged=process['cpu_nanos']-old['cpu_nanos']
  if ticks<=0 or charged<0:continue
  threads={item['tid']:item['cpu_nanos'] for item in old['threads']}
  deltas=[(0,process['main_cpu_nanos']-old['main_cpu_nanos'])]
  deltas.extend((item['tid'],item['cpu_nanos']-threads[item['tid']])
      for item in process['threads'] if item['tid'] in threads and item['cpu_nanos']>=threads[item['tid']])
  blocked=[task for task in sample['tasks'] if task['process']==process['address'] and task['blocked']]
  runnable=[task for task in sample['tasks'] if task['process']==process['address'] and not task['blocked']]
  intervals.append(dict(pid=pid,host_elapsed=round(sample['elapsed'],2),guest_seconds=round(ticks/100,2),
      cpu_charged_seconds=round(charged/1e9,3),blocked=len(blocked) if blocked_coverage else None,runnable=len(runnable),
      busiest_tid_seconds=[(tid,round(nanos/1e9,3)) for tid,nanos in sorted(deltas,key=lambda x:x[1],reverse=True)[:4]]))
  totals[pid]=totals.get(pid,0)+charged
for item in intervals[-15:]:print('INTERVAL',json.dumps(item))
print('TOTALS',json.dumps({pid:round(value/1e9,3) for pid,value in totals.items()}))
