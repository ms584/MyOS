"""Diagnostic saved GPU interrupt-frame PCs, NOT an instruction-time profiler.

No general stack, app payload or key reads. Saved frames are stale while a task
runs; separate QMP reads are non-atomic. Only stable slot/frame/CPU observations
are retained; PIT vector32 PCs identify candidates, never exact CPU percentages.
"""
import json,re,subprocess,time
from test_system import symbols

class Observer:
 def __init__(self,vm,out):
  self.vm=vm;self.out=out;self.names=symbols();self.records=[]
  self.layout=json.loads(subprocess.check_output(['build/cobalt-gpu-frame-layout'],text=True))
 def capture(self,launch,elapsed):
  def value(raw,offset,width=8):return int.from_bytes(raw[offset:offset+width],'little')
  registers=self.vm.command('human-monitor-command',{'command-line':'info registers'})
  match=re.search(r'CR3=([0-9a-fA-F]+)',registers)
  assert match,'missing diagnostic CR3'
  root=int(match[1],16)&0x000ffffffffff000
  def virtual(address,length):
   result=bytearray()
   while length:
    table=root;span=4096
    for shift in (39,30,21,12):
     entry=value(self.vm.memory(table+((address>>shift)&511)*8,8),0)
     if not entry&1:return None
     if shift in (30,21) and entry&128:
      span=1<<shift;physical=(entry&0x000ffffffffff000&~(span-1))+(address&(span-1));break
     table=entry&0x000ffffffffff000
     if shift==12:physical=table+(address&4095)
    count=min(length,span-(address&(span-1)))
    result.extend(self.vm.memory(physical,count));address+=count;length-=count
   return bytes(result)
  task=self.layout['task'];thread=self.layout['thread'];frame=self.layout['frame']
  before=value(self.vm.memory(self.names['ticks'],8),0)
  raw=self.vm.memory(self.names['tasks'],task['size']*task['count'])
  for index in range(task['count']):
   item=raw[index*task['size']:(index+1)*task['size']]
   pointer=value(item,task['thread']);saved=value(item,task['frame'])
   if not pointer or not saved:continue
   first=virtual(pointer,thread['size'])
   if first is None:continue
   tid=value(first,thread['tid'])
   if tid&0xffffffff!=11:continue
   # low11 identified by actual native log as Chrome_InProcGpuThread; record tid
   # so a future changed engine/thread order cannot silently reuse this label.
   state=virtual(saved,frame['size'])
   second=virtual(pointer,thread['size'])
   slot=self.vm.memory(self.names['tasks']+index*task['size'],task['size'])
   stable=second is not None and value(second,thread['tid'])==tid and value(second,thread['cpu_nanos'])==value(first,thread['cpu_nanos']) and value(slot,task['frame'])==saved and value(slot,task['thread'])==pointer
   if state is None:continue
   self.records.append(dict(launch=launch,elapsed=elapsed,host_monotonic=time.monotonic(),tid=tid,
    cpu_nanos=value(first,thread['cpu_nanos']),rip=value(state,frame['rip']),
    cs=value(state,frame['cs']),vector=value(state,frame['vector']),
    stable_observation=stable,atomic_snapshot=False,
    guest_tick_before=before,guest_tick_after=value(self.vm.memory(self.names['ticks'],8),0)))
  (self.out/'gpu-saved-frames.json').write_text(json.dumps(self.records,indent=2))
