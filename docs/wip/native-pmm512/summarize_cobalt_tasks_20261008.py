import json,sys,subprocess
from pathlib import Path
root=Path('/home/xytron404/cobalt-closed-imports-xnVZeH')
data=json.loads(Path(sys.argv[1]).read_text())
for task in data['tasks']:
    regs=task.get('registers',{});rip=regs.get('rip',0)
    print('slot',task['slot'],'tid',hex(task.get('tid',0)),
          'alive',task['alive'],'blocked',task.get('blocked'),
          'rip',hex(rip),'wait',hex(task.get('wait_address',0)),
          'deadline',task.get('wait_deadline',0))
    if task.get('process') and (task['alive'] or (task.get('tid',0)&0xffffffff) in (5,11,16,18)):
        addresses=[rip]+[x['address'] for x in task.get('stack_candidates',[])[:8]]
        for address in addresses:
            if 0x8006331000<=address<0x8006331000+0x1000000:
                binary=root/'build/renderer-provider-evaluation/provider-84sokh6g/provider.so';offset=address-0x8006331000
            elif 0x8000000000<=address<0x8006331000:
                binary=root/'build/engine-evaluation/libcobalt.so';offset=address-0x8000000000
            else:continue
            result=subprocess.check_output(['addr2line','-f','-C','-e',str(binary),hex(offset)],text=True).strip()
            print(' ',hex(address),hex(offset),result.replace('\n',' | '))
