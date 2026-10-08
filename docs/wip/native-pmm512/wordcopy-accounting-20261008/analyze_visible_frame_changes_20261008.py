"""Pixel changes in fixed timer/input regions, not OCR or exact render timing."""
from pathlib import Path
from PIL import Image
import hashlib,json,re,sys
root=Path(sys.argv[1]);result=[]
for launch in (1,2):
    rows=[];prior={};changes={'timer':[],'input':[]}
    for p in sorted(root.glob('launch-%d-frame-*.ppm'%launch)):
        elapsed=int(re.search(r'frame-(\d+)',p.name)[1]);im=Image.open(p)
        # These regions are valid only for the pinned 800x600 deterministic page.
        assert im.size==(800,600)
        colors=im.getdata();blue=sum(r<230 and g>220 and b>235 for r,g,b in colors)
        if blue<10000:continue
        row={'elapsed':elapsed,'blue_pixels':blue}
        for label,box in [('timer',(24,230,775,280)),('input',(24,410,775,465))]:
            digest=hashlib.sha256(im.crop(box).tobytes()).hexdigest()
            row[label+'_sha256']=digest
            if prior.get(label)!=digest:changes[label].append(elapsed)
            prior[label]=digest
        rows.append(row)
    result.append(dict(launch=launch,samples=rows,region_change_sample_times=changes,
                       caveat='10s sampled pixels; no OCR; values/latency require actual frames and event journal'))
target=root/'visible-frame-change-analysis.json'
target.write_text(json.dumps(result,indent=2))
for r in result:print(r['launch'],r['region_change_sample_times'])
if (root/'evidence.json').exists():
    e=json.loads((root/'evidence.json').read_text())
    print('INPUTS',json.dumps(e['inputs']))
    print('EVENTS',json.dumps([r for r in e['requests'] if 'kind=key' in r['path'] or 'kind=click' in r['path']]))
