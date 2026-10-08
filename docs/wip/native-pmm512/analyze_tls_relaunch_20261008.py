from pathlib import Path
from collections import Counter
import json,sys
from urllib.parse import urlsplit, parse_qs
root=Path('/home/xytron404/cobalt-closed-imports-xnVZeH/build')/(sys.argv[1] if len(sys.argv)>1 else 'native-verified-tls-0dgifhbw')
data=json.loads((root/'evidence.json').read_text()) if (root/'evidence.json').exists() else dict(
 requests=[json.loads(line) for line in (root/'request-events.jsonl').read_text().splitlines()],
 launches=json.loads((root/'launches.json').read_text()))
for launch in data['launches']:
    events=data['requests'][launch['request_start']:launch['request_end']]
    counts=Counter()
    print('LAUNCH',launch)
    for event in events:
        query=parse_qs(urlsplit(event['path']).query)
        kind=query.get('kind',[urlsplit(event['path']).path])[0]
        counts[kind]+=1
        if kind in ('asset-error','asset-ok','key','click','certificate-rejected'):
            print('EVENT',event)
    print('COUNTS',dict(counts))
serial=root/'serial.txt'
if not serial.exists():serial=root.parent/'system-tests/native-verified-tls.log'
lines=serial.read_text().splitlines()
print('SELECTED NATIVE ERRORS')
for index,line in enumerate(lines):
    if 'ERROR:' in line and ('Creating shared memory' in line or 'net_error' in line):
        print(line)
print('INVALID HTTP',sum(event['server']!='valid' for event in data['requests']))
