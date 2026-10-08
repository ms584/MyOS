from pathlib import Path
import json,sys
from PIL import Image
root=Path(sys.argv[1]);data=json.loads((root/'evidence.json').read_text())
print('diagnostic',data['diagnostic_only'],'launches',data['launches'],'inputs',len(data['inputs']),
 'frames',len(data['visual']),'max blue',max(r['blue_pixels'] for r in data['visual']))
print('provider',data['provider_sha256'])
target=Path(__file__).parent/(root.name+'-final.png')
Image.open(root/'launch-1-final.ppm').save(target)
print('Unmodified screenshot format conversion',target)
