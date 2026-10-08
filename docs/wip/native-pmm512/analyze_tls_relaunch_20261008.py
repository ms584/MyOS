from pathlib import Path
from collections import Counter
import json
from urllib.parse import urlsplit, parse_qs
root=Path('/home/xytron404/cobalt-closed-imports-xnVZeH/build/native-verified-tls-0dgifhbw')
data=json.loads((root/'evidence.json').read_text())
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
lines=(root/'serial.txt').read_text().splitlines()
print('SELECTED NATIVE ERRORS')
for index,line in enumerate(lines):
    if 'ERROR:' in line and ('ssl_client' not in line or 'net_error' in line):
        print(line)
print('INVALID HTTP',sum(event['server']!='valid' for event in data['requests']))
