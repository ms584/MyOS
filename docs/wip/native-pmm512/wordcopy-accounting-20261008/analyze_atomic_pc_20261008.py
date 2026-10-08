"""Containing-symbol PC sample counts only; no nearest-export/CPU% claims."""
from pathlib import Path
from collections import Counter
import subprocess,json,re,sys
root=Path('/home/xytron404/cobalt-closed-imports-xnVZeH')
bundle=Path(sys.argv[1]);records=json.loads((bundle/'atomic-current-pcs.json').read_text())
serial=(bundle/'serial.txt').read_text() if (bundle/'serial.txt').exists() else (root/'build/system-tests/native-verified-tls.log').read_text()
match=re.search(r'ELF pair provider bias: (0x[0-9a-fA-F]+)',serial);assert match
bias=int(match[1],16);tables=[];exact_symbols={}
for module,path,offset in [('kernel',root/'build/kernel.elf',0),('provider',root/'build/renderer-provider-evaluation/provider-84sokh6g/provider.so',bias),('runtime',root/'build/platform-runtime.elf',0x803f000000)]:
 entries=[]
 for line in subprocess.check_output(['nm','-S','--defined-only',str(path)],text=True).splitlines():
  f=line.split()
  if len(f)==3 and f[1] in ('t','T','w','W'):
   exact_symbols[int(f[0],16)+offset]=module+':'+f[2]+' (exact symbol)'
   continue
  if len(f)!=4 or f[2] not in ('t','T','w','W'):continue
  start,size=int(f[0],16),int(f[1],16)
  if module!='runtime' or start<offset:start+=offset
  if size:entries.append((start,start+size,f[3]))
 tables.append((module,entries))
counts=Counter();threads=Counter()
for row in records:
 assert row['atomic_snapshot'];pc=row['rip'];label=None
 for module,entries in tables:
  found=[e for e in entries if e[0]<=pc<e[1]]
  if found:
   e=min(found,key=lambda e:e[1]-e[0]);label=module+':'+e[2];break
 if label is None:label=exact_symbols.get(pc)
 if label is None:label=('core+'+hex(pc-0x8000000000)) if 0x8000000000<=pc<bias else 'unresolved:'+hex(pc)
 counts[label]+=1;threads[row['tid']]+=1
result=dict(samples=len(records),counts=counts.most_common(),
            thread_metadata_valid=False,
            thread_metadata_error='Observer bound unrelated 192-byte current symbol, not scheduler current.lto_priv.0; exclude task/tid. CPU registers remain paused snapshots.',
            total_host_pause_seconds=sum(r['host_pause_seconds'] for r in records),
            caveat='Paused random wall samples perturb scheduling; no exact CPU percentages or acceptance.')
(bundle/'atomic-pc-analysis.json').write_text(json.dumps(result,indent=2))
print(json.dumps(result,indent=2))
