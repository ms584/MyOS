"""Read-only scheduler metadata; no VM stop, stacks, app payloads or keys.

Counters are 100Hz PIT-sampled guest CPU charges, NOT host CPU percentages.
Separate QMP reads are not an atomic scheduler snapshot. Observe tick range.
"""
import json,subprocess,time
from test_system import symbols

class Observer:
 def __init__(self,vm,out):
  self.vm=vm;self.out=out;self.names=symbols()
  self.layout=json.loads(subprocess.check_output(['build/cobalt-cpu-layout'],text=True))
  self.records=[]
 def capture(self,launch,elapsed):
  def value(raw,offset,width=8):return int.from_bytes(raw[offset:offset+width],'little')
  tick_before=value(self.vm.memory(self.names['ticks'],8),0)
  p=self.layout['process'];t=self.layout['thread'];task=self.layout['task']
  raw=self.vm.memory(self.names['processes'],p['size']*p['count'])
  processes=[]
  for i in range(p['count']):
   item=raw[i*p['size']:(i+1)*p['size']]
   if value(item,p['state'],4)!=1:continue
   entry={key:value(item,p[key]) for key in ('pid','cpu_nanos','main_cpu_nanos','preemptions')}
   entry['address']=self.names['processes']+i*p['size'];entry['threads']=[]
   for j in range(t['count']):
    offset=p['threads']+j*t['size'];thread=item[offset:offset+t['size']]
    if not value(thread,t['state'],4):continue
    entry['threads'].append(dict(tid=value(thread,t['tid']),state=value(thread,t['state'],4),
        cpu_nanos=value(thread,t['cpu_nanos']),address=entry['address']+offset))
   processes.append(entry)
  raw=self.vm.memory(self.names['tasks'],task['size']*task['count']);tasks=[]
  for i in range(task['count']):
   item=raw[i*task['size']:(i+1)*task['size']]
   if not value(item,task['alive'],1) and not value(item,task['blocked'],1):continue
   tasks.append(dict(slot=i,process=value(item,task['process']),thread=value(item,task['thread']),
       blocked=value(item,task['blocked'],1),wait_deadline=value(item,task['wait_deadline'])))
  self.records.append(dict(launch=launch,elapsed=elapsed,host_monotonic=time.monotonic(),
      guest_tick_before=tick_before,guest_tick_after=value(self.vm.memory(self.names['ticks'],8),0),
      processes=processes,tasks=tasks,atomic_snapshot=False,blocked_task_coverage=True))
  (self.out/'cpu-accounting.json').write_text(json.dumps(self.records,indent=2))
