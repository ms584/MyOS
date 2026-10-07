"""Read-only paused QEMU snapshot; diagnostic, never timing/acceptance proof."""
import json,subprocess
from pathlib import Path
from test_system import symbols
def capture(vm,out):
 layout=json.loads(subprocess.check_output(['build/cobalt-task-layout'],text=True))
 names=symbols();tasks=layout['task'];processes=layout['process'];frame=layout['frame']
 result={'diagnostic_only':True,'tasks':[],'processes':[]}
 def value(data,offset,width=8):return int.from_bytes(data[offset:offset+width],'little')
 vm.command('stop')
 try:
  raw=vm.memory(names['processes'],processes['size']*processes['count']);(out/'processes.bin').write_bytes(raw)
  for index in range(processes['count']):
   item=raw[index*processes['size']:(index+1)*processes['size']]
   if not value(item,processes['pid']):continue
   result['processes'].append({key:value(item,processes[key],4 if key in ('state','vm_page_count') else 8) for key in ['pid','state','root','vm_page_count','vm_areas']})
  raw=vm.memory(names['tasks'],tasks['size']*tasks['count']);(out/'tasks.bin').write_bytes(raw)
  for index in range(tasks['count']):
   item=raw[index*tasks['size']:(index+1)*tasks['size']]
   if not value(item,tasks['alive'],1):continue
   entry={'slot':index}
   for key in ['frame','process','thread','blocked','wait_address','wait_deadline']:
    entry[key]=value(item,tasks[key],1 if key=='blocked' else 8)
   if entry['thread']:entry['tid']=value(vm.memory(entry['thread']+layout['thread']['tid'],8),0)
   if entry['frame']:
    saved=vm.memory(entry['frame'],frame['size']);entry['registers']={key:value(saved,frame[key]) for key in ['rip','rsp','rbp','rdi','rsi','rdx','rax']}
   result['tasks'].append(entry)
 finally:vm.command('cont')
 (out/'task-snapshot.json').write_text(json.dumps(result,indent=2))
 print('DIAGNOSTIC TASK SNAPSHOT',out/'task-snapshot.json',flush=True)
