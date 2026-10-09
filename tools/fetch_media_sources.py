"""Fetch only hash-pinned source archives and the standard native test asset."""
from pathlib import Path
import hashlib,json,urllib.request
root=Path(__file__).resolve().parents[1]
base=root/'build/media-evaluation';base.mkdir(parents=True,exist_ok=True)
lock=json.loads((root/'docs/PHASE333_MEDIA_LOCK.json').read_text())
items=[*lock['dependencies'],dict(lock['fixture'],archive=lock['fixture']['name'])]
for item in items:
 path=base/item['archive']
 if path.exists() and hashlib.sha256(path.read_bytes()).hexdigest()==item['sha256']:
  print('VERIFIED',path,flush=True);continue
 data=urllib.request.urlopen(item['url'],timeout=90).read()
 assert hashlib.sha256(data).hexdigest()==item['sha256'],item['archive']
 temporary=path.with_suffix(path.suffix+'.partial');temporary.write_bytes(data);temporary.replace(path)
 print('FETCHED/VERIFIED',path,flush=True)
print('Sources/assets only; no host or native media decoding executed.')
