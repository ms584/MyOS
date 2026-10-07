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
  kernel_root=next(item['root'] for item in result['processes'] if item['root'])
  roots={names['processes']+index*processes['size']:value(raw[index*processes['size']:(index+1)*processes['size']],processes['root']) for index in range(processes['count'])}
  def virtual(root,address,length):
   result_bytes=bytearray()
   while length:
    table=root;physical=None;span=4096
    for shift in (39,30,21,12):
     entry=value(vm.memory(table+((address>>shift)&511)*8,8),0)
     if not entry&1:raise RuntimeError(('unmapped diagnostic VA',hex(address)))
     if shift in (30,21) and entry&128:
      span=1<<shift;physical=(entry&0x000ffffffffff000&~(span-1))+(address&(span-1));break
     table=entry&0x000ffffffffff000
     if shift==12:physical=table+(address&4095)
    count=min(length,span-(address&(span-1)))
    result_bytes.extend(vm.memory(physical,count));address+=count;length-=count
   return bytes(result_bytes)
  raw=vm.memory(names['tasks'],tasks['size']*tasks['count']);(out/'tasks.bin').write_bytes(raw)
  for index in range(tasks['count']):
   item=raw[index*tasks['size']:(index+1)*tasks['size']]
   alive=value(item,tasks['alive'],1);blocked=value(item,tasks['blocked'],1)
   if not alive and not blocked:continue
   entry={'slot':index,'alive':alive}
   for key in ['frame','process','thread','blocked','wait_address','wait_deadline']:
    entry[key]=value(item,tasks[key],1 if key=='blocked' else 8)
   if entry['thread']:entry['tid']=value(vm.memory(entry['thread']+layout['thread']['tid'],8),0)
   if entry['frame']:
    saved=virtual(kernel_root,entry['frame'],frame['size']);entry['registers']={key:value(saved,frame[key]) for key in ['rip','rsp','rbp','rdi','rsi','rdx','rax']}
   if entry['process'] and entry['wait_address']:
    try:entry['wait_bytes']=virtual(roots[entry['process']],entry['wait_address'],32).hex()
    except RuntimeError as error:entry['wait_read_error']=str(error)
   if entry['process'] and entry.get('registers',{}).get('rsp',0)>=0x8000000000:
    try:
     stack=virtual(roots[entry['process']],entry['registers']['rsp'],256)
     entry['stack_bytes']=stack.hex()
     # Potential return pointers only; these are NOT asserted full backtraces.
     entry['stack_candidates']=[{'offset':offset,'address':value(stack,offset)} for offset in range(0,len(stack),8) if
      0x8000000000<=value(stack,offset)<0x8008000000 or
      0x803f000000<=value(stack,offset)<0x803f010000 or
      0x8800000000<=value(stack,offset)<0x8801000000]
    except RuntimeError as error:entry['stack_read_error']=str(error)
   result['tasks'].append(entry)
 finally:vm.command('cont')
 (out/'task-snapshot.json').write_text(json.dumps(result,indent=2))
 print('DIAGNOSTIC TASK SNAPSHOT',out/'task-snapshot.json',flush=True)
