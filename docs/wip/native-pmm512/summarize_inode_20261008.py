from pathlib import Path
from collections import Counter
import json,sys
root=Path('/home/xytron404/cobalt-closed-imports-xnVZeH/build')/sys.argv[1]
for file in sorted(root.glob('resources-*.json')):
    data=json.loads(file.read_text());nodes=data['nodes']
    print(file.name,'nodes',len(nodes),'heap',data['heap_allocated'],
        'shared',sum(bool(n['shared']) for n in nodes),
        'unlinked',sum(n['unlinked'] for n in nodes),
        'capacity',sum(n['capacity'] for n in nodes if n['data']))
    print('NODE TYPES/PARENTS',dict(Counter((n['kind'],n['parent']) for n in nodes)))
    for node in nodes:
        if node['shared'] or node['unlinked']:
            print('SHARED/UNLINKED',node)
    if 'active' in file.name:
        for node in nodes:
            if not node['shared'] and not node['unlinked']:
                print('NAMED',node['index'],node['parent'],node['name'],node['size'])
