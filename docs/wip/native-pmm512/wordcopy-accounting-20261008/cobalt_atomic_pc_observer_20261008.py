"""Paused current CPU PC samples, not stale saved frames or acceptance evidence.

Pauses perturb scheduling. Samples identify containing functions, not exact CPU
percentages. Only CPU registers and kernel scheduler metadata are inspected.
"""
import json,re,subprocess,time
from test_system import symbols
class Observer:
 def __init__(self,vm,out):
  self.vm=vm;self.out=out;self.names=symbols();self.records=[]
  self.layout=json.loads(subprocess.check_output(['build/cobalt-gpu-frame-layout'],text=True))
  assert 'current' in self.names and 'tasks' in self.names
 def capture(self,launch,elapsed):
  started=time.monotonic()
  assert self.vm.command('query-status')['running']
  self.vm.command('stop')
  try:
   assert not self.vm.command('query-status')['running']
   registers=self.vm.command('human-monitor-command',{'command-line':'info registers'})
   def reg(name):
    match=re.search(r'\b'+name+r'=\s*([0-9a-fA-F]+)',registers)
    assert match,('missing register',name)
    return int(match[1],16)
   cr3=reg('CR3')&0x000ffffffffff000
   def value(raw,offset=0,width=8):return int.from_bytes(raw[offset:offset+width],'little')
   def virtual(address,length):
    result=bytearray()
    while length:
     table=cr3;span=4096
     for shift in (39,30,21,12):
      entry=value(self.vm.memory(table+((address>>shift)&511)*8,8))
      assert entry&1,'unmapped kernel metadata'
      if shift in (30,21) and entry&128:
       span=1<<shift;physical=(entry&0x000ffffffffff000&~(span-1))+(address&(span-1));break
      table=entry&0x000ffffffffff000
      if shift==12:physical=table+(address&4095)
     count=min(length,span-(address&(span-1)))
     result.extend(self.vm.memory(physical,count));address+=count;length-=count
    return bytes(result)
   task=self.layout['task'];thread=self.layout['thread']
   current=value(self.vm.memory(self.names['current'],4),width=4)
   assert current<task['count']
   raw=self.vm.memory(self.names['tasks']+current*task['size'],task['size'])
   pointer=value(raw,task['thread']);tid=0
   if pointer:tid=value(virtual(pointer,thread['size']),thread['tid'])
   row=dict(launch=launch,elapsed=elapsed,rip=reg('RIP'),cs=reg('CS'),cr3=cr3,
            task=current,tid=tid,atomic_snapshot=True,diagnostic_only=True)
  finally:self.vm.command('cont')
  row['host_pause_seconds']=time.monotonic()-started;self.records.append(row)
  (self.out/'atomic-current-pcs.json').write_text(json.dumps(self.records,indent=2))
