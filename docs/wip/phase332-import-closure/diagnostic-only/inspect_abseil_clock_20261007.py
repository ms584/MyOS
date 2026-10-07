import hashlib,json,urllib.request
from pathlib import Path
root=Path('/home/xytron404/myos')
pin=json.loads((root/'docs/PHASE33_COBALT_AUDIT.json').read_text())['source_commit']
name='third_party/abseil-cpp/absl/base/internal/sysinfo.cc'
target=root/'build/cobalt-font-audit-20261007/abseil-sysinfo.cc'
if not target.exists():
 with urllib.request.urlopen(f'https://raw.githubusercontent.com/youtube/cobalt/{pin}/{name}',timeout=25) as response:target.write_bytes(response.read())
data=target.read_bytes();print(pin,name,hashlib.sha256(data).hexdigest())
for n,line in enumerate(data.decode().splitlines(),1):
 if 214<=n<=267:print(f'{n}: {line}')
