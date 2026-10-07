import json,sys
from pathlib import Path
data=json.loads(Path(sys.argv[1]).read_text())
print('Diagnostic only; saved scheduler frames, not full backtraces.')
for task in data['tasks']:
 if not task['process']:continue
 registers=task.get('registers',{})
 print('slot',task['slot'],'tid',hex(task.get('tid',0)),
       'alive',task['alive'],'blocked',task['blocked'],
       'rip',hex(registers.get('rip',0)),'rax',hex(registers.get('rax',0)),
       'wait',hex(task['wait_address']),'deadline',task['wait_deadline'],
       'bytes',task.get('wait_bytes','-'))
