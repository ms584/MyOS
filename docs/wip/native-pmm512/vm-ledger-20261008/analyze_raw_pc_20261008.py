"""Containing provider symbols only; kernel/stripped gaps stay unclassified."""
from pathlib import Path
from collections import Counter
from bisect import bisect_right
import hashlib,json,re,subprocess,sys
bundle=Path(sys.argv[1]);evidence=json.loads((bundle/'evidence.json').read_text())
provider=Path(evidence['provider_path'])
assert provider.name=='provider.so'
assert hashlib.sha256(provider.read_bytes()).hexdigest()==evidence['provider_sha256']
serial=(bundle/'serial.txt').read_text()
bases={int(m,16) for m in re.findall(r'ELF pair provider bias:\s*(0x[0-9a-fA-F]+)',serial)}
assert len(bases)==1,bases
base=bases.pop();symbols={}
for line in subprocess.check_output(['nm','-S','--defined-only',str(provider)],text=True).splitlines():
    fields=line.split(maxsplit=3)
    if len(fields)!=4 or fields[2] not in ('t','T','w','W'):continue
    try:address=int(fields[0],16);size=int(fields[1],16)
    except ValueError:continue
    if size:symbols.setdefault(address,[]).append((size,fields[3]))
starts=sorted(symbols)
def read_symbols(path):
    found={}
    for line in subprocess.check_output(['nm','-S','--defined-only',str(path)],text=True).splitlines():
        fields=line.split(maxsplit=3)
        if len(fields)!=4 or fields[2] not in ('t','T','w','W'):continue
        try:address=int(fields[0],16);size=int(fields[1],16)
        except ValueError:continue
        if size:found.setdefault(address,[]).append((size,fields[3]))
    return sorted(found),found
def containing(table,address):
    ordered,found=table;index=bisect_right(ordered,address)-1
    candidates=[name for size,name in found[ordered[index]] if ordered[index]<=address<ordered[index]+size] if index>=0 else []
    return sorted(candidates)[0] if candidates else None
kernel=runtime=None;matched_hashes={};kernel_labels={}
if len(sys.argv)==3:
    matched=Path(sys.argv[2])
    pins={'myos-current.img':'978d15abe933379c1dd5a9a89fde59f08fe306d1a79a60bad41a06f70cca1313',
          'kernel.elf':'d1b39ae922475991de36554d2a4c7c2c94f9fe7b61ecccfadc597a1b51187b0b',
          'platform-runtime.elf':'07a1014a92f379780578b70d3e38c3adf98c0926ae2fb197c79ea9abed37c388'}
    for name,expected in pins.items():
        actual=hashlib.sha256((matched/'build'/name).read_bytes()).hexdigest()
        assert actual==expected,(name,actual)
        matched_hashes[name]=actual
    kernel=read_symbols(matched/'build/kernel.elf')
    runtime=read_symbols(matched/'build/platform-runtime.elf')
    for line in subprocess.check_output(['nm','--defined-only',str(matched/'build/kernel.elf')],text=True).splitlines():
        fields=line.split(maxsplit=2)
        if len(fields)==3 and fields[1] in ('t','T'):
            kernel_labels.setdefault(int(fields[0],16),[]).append(fields[2])
counts=Counter();samples=json.loads((bundle/'raw-current-pc.json').read_text())
for sample in samples:
    assert sample['atomic_register_snapshot'] and not sample['task_identity_valid']
    if sample['cs']&3!=3:
        name=containing(kernel,sample['rip']) if kernel else None
        sample['classification']='kernel: '+name if name else 'kernel: unclassified (no containing sized symbol)'
        if matched_hashes and sample['rip']==0x21fc9:
            sample['classification']='kernel: post-HLT launcher idle boundary (not busy work)'
        elif not name and sample['rip'] in kernel_labels:
            sample['classification']='kernel exact label only: '+','.join(sorted(kernel_labels[sample['rip']]))
    else:
        address=sample['rip']-base;index=bisect_right(starts,address)-1
        candidates=[name for size,name in symbols[starts[index]] if starts[index]<=address<starts[index]+size] if index>=0 else []
        runtime_name=containing(runtime,sample['rip']) if runtime else None
        sample['classification']='provider: '+sorted(candidates)[0] if candidates else 'runtime: '+runtime_name if runtime_name else 'user: unclassified (stripped core/gap)'
    counts[sample['classification']]+=1
result=dict(provider_base=hex(base),provider_sha256=evidence['provider_sha256'],
    samples=len(samples),pause_seconds=sum(s['pause_seconds'] for s in samples),
    containing_symbol_counts=counts.most_common(),matched_elf_hashes=matched_hashes,
    most_common_raw_pcs=[dict(rip=hex(rip),count=count) for rip,count in Counter(s['rip'] for s in samples).most_common(20)],
    warning='QMP/TCG sampling is biased. Not CPU percentages; no task/thread attribution; no nearest-symbol fallback.')
(bundle/'raw-pc-analysis.json').write_text(json.dumps(result,indent=2))
print(json.dumps(result,indent=2))
