"""Evidence gate for native web behavior only, not the full phase/regression gate."""
import json,re,sys,urllib.parse
from pathlib import Path
from PIL import Image
root=Path(sys.argv[1]).resolve()
e=json.loads((root/'evidence.json').read_text())
assert not e['diagnostic_only'] and len(e['launches'])>=2
assert e['peak_concurrent_assets']>=3
assert all(r['server']=='valid' for r in e['requests'])
serial=(root/'serial.txt').read_text()
assert 'PANIC' not in serial
residents=re.findall(r'ELF resident: free-before=(\d+) free-after=(\d+) shm-before=(\d+) shm-after=(\d+) aliases-before=(\d+) aliases-after=(\d+)',serial)
assert len(residents)>=len(e['launches'])
for values in residents:
    before,after,shm1,shm2,alias1,alias2=map(int,values)
    assert before==after and shm1==shm2==alias1==alias2==0
heaps=[];report=[]
for launch in e['launches']:
    number=launch['launch'];assert launch['shell_returned']
    assert any('all threads stopped' in line for line in launch['cleanup'])
    heap=next(re.search(r'heap-before=(\d+) heap-after=(\d+)',line) for line in launch['cleanup'] if 'heap-before=' in line)
    heaps.append(int(heap[2]))
    records=e['requests'][launch['request_start']:launch['request_end']]
    events=[urllib.parse.parse_qs(urllib.parse.urlsplit(x['path']).query) for x in records]
    def values(kind):return [x['value'][0] for x in events if x.get('kind')==[kind]]
    assert '42' in values('dom') and '1' in values('click')
    assert {'wrong-host','expired','untrusted'}<=set(values('certificate-rejected'))
    assert {'0','1','2'}<=set(values('asset-ok'))
    assert {'80','0'}<=set(values('keyboard-js-scroll'))
    timers=list(map(int,values('timer')));assert max(timers)-min(timers)>=60
    frames=[]
    for item in e['visual']:
        if item['launch']!=number or not item.get('marker_present'):continue
        path=(root/item['frame']).resolve();assert path.parent==root
        with Image.open(path) as picture:
            picture=picture.convert('RGB')
            assert picture.getpixel((636,14))==(255,0,255)
            for field,y in (('ack',14),('clock',34)):
                colors=[picture.getpixel((656+16*b,y)) for b in range(8)]
                assert all(c in ((0,0,0),(255,255,255)) for c in colors)
                actual=sum(1<<b for b,c in enumerate(colors) if c==(255,255,255))
                assert actual==item[field]
        frames.append(item)
    assert frames[-1]['elapsed']-frames[0]['elapsed']>=60
    assert any(x['ack']>=129 for x in frames), 'no visible click acknowledgement'
    latencies=[]
    for key,code in [('a',1),('b',2),('pgdn',3),('c',4),('pgup',5)]:
        sent=next(x for x in e['inputs'] if x['launch']==number and x.get('key')==key)
        acknowledged=next(x for x in frames if x['elapsed']>=sent['elapsed'] and x['ack']==code)
        upper=acknowledged['capture_finished']-sent['elapsed']
        assert upper<=2.0, (number,key,upper)
        latencies.append(dict(key=key,sampled_upper_seconds=upper))
    report.append(dict(launch=number,visible_span_seconds=frames[-1]['elapsed']-frames[0]['elapsed'],first_visible_seconds=frames[0]['elapsed'],keys=latencies))
assert heaps[-1]<=heaps[0], 'heap increased after warm close'
result=dict(native_web_evidence_pass=True,launches=report,heap_after=heaps,
            caveat='Native web subset only; integrated regression, source promotion, sampled RAM review and final docs remain independent gates. Key bound2s includes1s image sampling.')
(root/'native-web-verification.json').write_text(json.dumps(result,indent=2))
print(json.dumps(result,indent=2))
