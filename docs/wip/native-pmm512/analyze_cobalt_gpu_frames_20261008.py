"""Candidate saved-PC locations, not exact CPU attribution or acceptance."""
from pathlib import Path
from collections import Counter
import json,re,subprocess,sys

root=Path('/home/xytron404/cobalt-closed-imports-xnVZeH')
name=sys.argv[1] if len(sys.argv)>1 else 'auto'
if name=='auto':
 matches=re.findall(r'native-verified-tls-([a-zA-Z0-9_]+)',(root/'cobalt-gpu-frames.log').read_text())
 assert matches,'missing live evidence bundle'
 name='native-verified-tls-'+matches[-1]
bundle=root/'build'/name
print('bundle',bundle)
records=json.loads((bundle/'gpu-saved-frames.json').read_text())
serial_file=bundle/'serial.txt'
if not serial_file.exists():serial_file=root/'build/system-tests/native-verified-tls.log'
serial=serial_file.read_text()
match=re.search(r'ELF pair provider bias: (0x[0-9a-fA-F]+)',serial)
assert match,'missing actual provider bias'
provider_bias=int(match[1],16)
tables=[]
for name,path,bias in (
 ('kernel',root/'build/kernel.elf',0),
 ('provider',root/'build/renderer-provider-evaluation/provider-84sokh6g/provider.so',provider_bias),
 ('runtime',root/'build/platform-runtime.elf',0x803f000000)):
 if not path.exists():continue
 entries=[]
 for line in subprocess.check_output(['nm','-S','--defined-only',str(path)],text=True).splitlines():
  fields=line.split()
  if len(fields)!=4 or fields[2] not in ('t','T','w','W'):continue
  address,size=int(fields[0],16),int(fields[1],16)
  relocation=0 if name=='runtime' and address>=bias else bias
  if size:entries.append((address+relocation,address+relocation+size,fields[3]))
 tables.append((name,entries))
counts=Counter();unmapped=Counter();selected=[]
for item in records:
 if not item['stable_observation'] or item['vector']!=32:continue
 selected.append(item)
 pc=item['rip'];found=False
 for module,entries in tables:
  containing=[entry for entry in entries if entry[0]<=pc<entry[1]]
  if containing:
   # Containment only: never nearest exported symbol in a stripped image.
   start,end,name=min(containing,key=lambda entry:entry[1]-entry[0])
   counts[(module,name,hex(pc-start))]+=1;found=True;break
 if not found:
  module='core' if 0x8000000000<=pc<provider_bias else 'unknown'
  unmapped[(module,hex(pc-(0x8000000000 if module=='core' else 0)))]+=1
print('diagnostic saved interrupt PCs; stale/non-atomic; NOT CPU percentages')
print('records',len(records),'stable PIT32',len(selected),'actual tids',sorted(set(item['tid'] for item in records)))
for key,count in counts.most_common(25):print('CONTAINED',count,*key)
for key,count in unmapped.most_common(15):print('UNRESOLVED',count,*key)
