from collections import Counter
import json,sys,urllib.parse
from pathlib import Path
entries=json.loads((Path(sys.argv[1])/'requests.json').read_text())
counts=Counter()
for entry in entries:
    query=urllib.parse.parse_qs(urllib.parse.urlsplit(entry['path']).query)
    kind=query.get('kind',['page'])[0];counts[kind]+=1
    if kind in ('dom','key','click','geometry'):
        print(kind,entry['elapsed'],query.get('value'))
print(dict(counts),'first/last',entries[0]['elapsed'],entries[-1]['elapsed'])
