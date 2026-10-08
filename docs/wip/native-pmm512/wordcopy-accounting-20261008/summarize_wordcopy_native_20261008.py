from pathlib import Path
import json
from urllib.parse import urlparse,parse_qs
from PIL import Image
root=Path('/home/xytron404/cobalt-closed-imports-xnVZeH/build/native-verified-tls-b7lanazn')
data=json.loads((root/'evidence.json').read_text())
print('provider',data['provider_sha256'],'peak concurrency',data['peak_concurrent_assets'])
starts=[0]+[x['request_end'] for x in data['launches'][:-1]]
for launch,start in zip(data['launches'],starts):
 number=launch['launch'];events=[]
 for row in data['requests'][start:launch['request_end']]:
  query=parse_qs(urlparse(row['path']).query)
  if query.get('kind',[''])[0] in ('dom','key','click','scroll','certificate-rejected','asset-ok'):
   events.append(dict(elapsed=row['elapsed'],query=query))
 print('launch',number,'cleanup',launch['cleanup'],'events',json.dumps(events))
 frames=[r for r in data['visual'] if r['launch']==number]
 print('first blue',next((r['elapsed'] for r in frames if r['blue_pixels']>10000),None))
 source=root/('launch-%d-final.ppm'%number)
 target=Path(__file__).parent/('wordcopy-launch-%d-final.png'%number)
 Image.open(source).save(target)
 print('unmodified screenshot format conversion',target)
