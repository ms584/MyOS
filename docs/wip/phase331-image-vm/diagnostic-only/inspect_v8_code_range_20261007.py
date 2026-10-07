import hashlib,json,urllib.request
from pathlib import Path
root=Path('/home/xytron404/myos')
pin=json.loads((root/'docs/PHASE33_COBALT_AUDIT.json').read_text())['source_commit']
out=root/'build/cobalt-v8-policy-audit-20261007';out.mkdir(exist_ok=True)
for name in ['v8/src/heap/code-range.cc','v8/src/flags/flag-definitions.h','cobalt/script/v8c/v8c_engine.cc']:
 target=out/name.replace('/','-')
 try:
  if not target.exists():
   with urllib.request.urlopen(f'https://raw.githubusercontent.com/youtube/cobalt/{pin}/{name}',timeout=25) as response:target.write_bytes(response.read())
 except Exception as error:
  print(name,type(error).__name__,str(error));continue
 data=target.read_bytes();lines=data.decode().splitlines()
 print(pin,name,hashlib.sha256(data).hexdigest())
 hits=set()
 for n,line in enumerate(lines):
  if any(word in line for word in ['write_protect_code_memory','ReadWriteExecute','js-flags','SetFlagsFrom','code_range','jitless']):
   hits.update(range(max(0,n-4),min(len(lines),n+6)))
 for n in sorted(hits):print(f'{n+1}: {lines[n]}')
