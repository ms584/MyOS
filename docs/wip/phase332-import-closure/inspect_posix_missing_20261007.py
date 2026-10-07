"""Pin the nine currently unavailable POSIX import signatures."""
import hashlib,json,urllib.request
from pathlib import Path
root=Path('/home/xytron404/myos')
pin=json.loads((root/'docs/PHASE33_COBALT_AUDIT.json').read_text())['source_commit']
out=root/'build/posix-missing-audit-20261007';out.mkdir(exist_ok=True)
for name in ('third_party/musl/include/sys/stat.h','third_party/musl/include/unistd.h','third_party/musl/include/sys/inotify.h'):
 target=out/name.replace('/','-')
 if not target.exists():
  with urllib.request.urlopen(f'https://raw.githubusercontent.com/youtube/cobalt/{pin}/{name}',timeout=25) as response:target.write_bytes(response.read())
 data=target.read_bytes();print(pin,name,hashlib.sha256(data).hexdigest(),flush=True)
 for line in data.decode().splitlines():
  if any(word in line for word in ('chmod','fchown','symlink (','readlink (','utimensat','inotify_')):print(line,flush=True)
