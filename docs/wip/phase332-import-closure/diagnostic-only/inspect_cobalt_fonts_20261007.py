"""Fetch only pinned sources selected by actual font initialization evidence."""
import hashlib,json,urllib.request,urllib.error
from pathlib import Path
root=Path('/home/xytron404/myos')
pin=json.loads((root/'docs/PHASE33_COBALT_AUDIT.json').read_text())['source_commit']
out=root/'build/cobalt-font-audit-20261007';out.mkdir(exist_ok=True)
for name in ('cobalt/renderer/rasterizer/skia/skia/src/ports/SkFontMgr_cobalt.cc',
             'cobalt/renderer/rasterizer/skia/skia/src/ports/SkFontConfigParser_cobalt.cc',
             'cobalt/renderer/rasterizer/skia/BUILD.gn',
             'third_party/musl/include/sys/resource.h'):
 target=out/name.replace('/','-')
 if not target.exists():
  try:
   with urllib.request.urlopen(f'https://raw.githubusercontent.com/youtube/cobalt/{pin}/{name}',timeout=25) as response:
    target.write_bytes(response.read())
  except urllib.error.HTTPError as error:
   print(name,'HTTP',error.code,flush=True);continue
 print(pin,name,len(target.read_bytes()),hashlib.sha256(target.read_bytes()).hexdigest(),target,flush=True)
