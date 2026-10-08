"""Analyze real metadata, preserving the first diagnostic heap-read limitation."""
from pathlib import Path
from collections import Counter
import json,subprocess,sys,bisect
root=Path('/home/xytron404/cobalt-closed-imports-xnVZeH/build')/sys.argv[1]
for file in sorted(root.glob('resources-*.json')):
    data=json.loads(file.read_text())
    nodes=data['nodes']
    print(file.name,'nodes',len(nodes),'regular_capacity',sum(n['capacity'] for n in nodes if n['data']),
        'shared_nodes',sum(bool(n['shared']) for n in nodes))
    # Kernel static metadata is identity-mapped. Heap pointers are virtual
    # 0x100000000: the first observer wrongly read those as physical addresses.
    # Its zero heap totals MUST NOT be used as resource-reclamation evidence.
    if not data.get('heap_virtual_translation'):
        print('INVALID_HEAP_TOTAL: first observer lacks virtual-to-physical translation')
    else:
        print('HEAP',data['heap_allocated'],'observed',data.get('heap_observed'))
    for node in nodes:
        if node['data'] or node['shared']:
            print('NODE',node)
evidence=root/'evidence.json'
if not evidence.exists():
    print('Still live: CPU sample summary deferred until terminal evidence')
    sys.exit(0)
data=json.loads(evidence.read_text());samples=data.get('cpu_samples',[])
provider=Path('/home/xytron404/cobalt-closed-imports-xnVZeH/build/renderer-provider-evaluation/provider-84sokh6g/provider.so')
symbols=[]
for line in subprocess.check_output(['nm','-S','--defined-only',str(provider)],text=True).splitlines():
    fields=line.split()
    if len(fields)==4 and fields[2] in ('t','T'):
        symbols.append((int(fields[0],16),int(fields[1],16),fields[3]))
symbols.sort();starts=[item[0] for item in symbols];counts=Counter()
for sample in samples:
    address=sample['rip'];relative=address-0x8006331000
    index=bisect.bisect_right(starts,relative)-1
    if index>=0 and starts[index]<=relative<starts[index]+symbols[index][1]:
        label='provider:'+symbols[index][2]
    elif 0x8000000000<=address<0x8006331000:label='core (stripped, no inferred function)'
    elif address<0x100000:label='kernel'
    else:label='other:'+hex(address)
    counts[label]+=1
print('DIAGNOSTIC RIP SAMPLES',len(samples),counts.most_common(16))
for image,bias,label in ((provider.parents[2]/'kernel.elf',0,'kernel'),
                         (provider.parents[2]/'platform-runtime.elf',0x803f000000,'runtime')):
    functions=[]
    for line in subprocess.check_output(['nm','-S','--defined-only',str(image)],text=True).splitlines():
        fields=line.split()
        if len(fields)==4 and fields[2] in ('t','T'):
            functions.append((int(fields[0],16),int(fields[1],16),fields[3]))
    counts=Counter()
    for sample in samples:
        relative=sample['rip']-bias
        matches=[item for item in functions if item[0]<=relative<item[0]+item[1]]
        if matches:counts[matches[0][2]]+=1
    print(label,counts.most_common(12))
