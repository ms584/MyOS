import hashlib,json,urllib.request
from pathlib import Path
root=Path('/home/xytron404/myos')
pin=json.loads((root/'docs/PHASE33_COBALT_AUDIT.json').read_text())['source_commit']
out=root/'build/cobalt-http-policy-audit-20261008';out.mkdir(exist_ok=True)
paths=['cobalt/browser/switches.h']
for name in paths:
 target=out/name.replace('/','-')
 try:
  if not target.exists():
   with urllib.request.urlopen(f'https://raw.githubusercontent.com/youtube/cobalt/{pin}/{name}',timeout=25) as response:target.write_bytes(response.read())
 except Exception as error:print(name,str(error),flush=True);continue
 data=target.read_bytes();lines=data.decode().splitlines()
 print(pin,name,hashlib.sha256(data).hexdigest(),flush=True)
 hits=set()
 for n,line in enumerate(lines):
  if any(word.lower() in line.lower() for word in ['EnforceHTTPS','EnforceCSP']):hits.update(range(max(0,n-3),min(len(lines),n+5)))
 for n in sorted(hits):print(f'{n+1}: {lines[n]}',flush=True)
