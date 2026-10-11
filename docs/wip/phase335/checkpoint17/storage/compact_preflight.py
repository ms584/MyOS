"""Guarded MyOS WSL trim/identity check; never shuts down or deletes files."""
from pathlib import Path
import hashlib,json,subprocess,sys
root=Path('/home/xytron404/myos')
r=subprocess.run(['pgrep','-af','qemu-system|ninja|test_system.py|observe_phase335.py|archive_old_mesa.py|build.sh|build_media_provider.py'],capture_output=True,text=True)
if r.returncode not in (0,1):
 raise RuntimeError('Cannot inspect active jobs; refusing compact.')
if r.stdout.strip():
 print('Refusing compact: an active test/build/cleanup exists.');raise SystemExit(2)
expected=json.loads((root/'docs/wip/phase335/checkpoint16/preserved-identities.json').read_text())['normal_identity']
for n,want in expected.items():
 h=hashlib.sha256()
 with (root/'build'/n).open('rb') as f:
  for chunk in iter(lambda:f.read(1048576),b''):h.update(chunk)
 assert h.hexdigest()==want,n
if len(sys.argv)>1 and sys.argv[1]=='verify':
 print('Normal five hashes exact after compact.');raise SystemExit(0)
subprocess.run(['sync'],check=True);subprocess.run(['fstrim','-v','/'],check=True)
print('Preflight: no active test/build; normal hashes exact; trim completed.')
