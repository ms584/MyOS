"""Visible-pixel latency bounds, not HTTP-receipt or throughput estimates."""
from pathlib import Path
import json,sys
root=Path(sys.argv[1]);e=json.loads((root/'evidence.json').read_text())
mapping={'a':1,'b':2,'pgdn':3,'c':4,'pgup':5}
results=[]
for sent in e['inputs']:
    code=mapping.get(sent.get('key'))
    if code is None:continue
    frames=[v for v in e['visual'] if v['launch']==sent['launch'] and v['elapsed']>=sent['elapsed']]
    match=next((v for v in frames if v.get('ack')==code),None)
    prior=[v for v in frames if match and v['elapsed']<match['elapsed'] and v.get('ack') is not None]
    results.append(dict(input=sent,code=code,acknowledged=bool(match),
        upper_seconds=match['capture_finished']-sent['elapsed'] if match else None,
        lower_seconds=max(0,prior[-1]['elapsed']-sent['elapsed']) if prior else 0,
        first_frame=match['frame'] if match else None))
clocks=[]
for launch in sorted({v['launch'] for v in e['visual']}):
    frames=[v for v in e['visual'] if v['launch']==launch and v.get('clock') is not None]
    changes=[];previous=None;longest=0
    for v in frames:
        if previous is None or v['clock']!=previous['clock']:
            if previous:longest=max(longest,v['capture_finished']-previous['elapsed'])
            changes.append(dict(elapsed=v['elapsed'],clock=v['clock']));previous=v
    if previous and frames:longest=max(longest,frames[-1]['capture_finished']-previous['elapsed'])
    clocks.append(dict(launch=launch,visible_changes=changes,longest_observed_plateau_seconds=longest))
report=dict(inputs=results,clocks=clocks,
    caveat='Sampled visible DOM markers; bounds include host input/screenshot overhead. Concurrent regression may affect timing. Not automatic phase acceptance.')
print(json.dumps(report,indent=2))
(root/'visual-latency-analysis.json').write_text(json.dumps(report,indent=2))
