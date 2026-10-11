"""Guarded MyOS WSL trim/identity check; never shuts down or deletes files."""
from pathlib import Path
import hashlib,json,subprocess,sys
root=Path('/home/xytron404/myos')
r=subprocess.run(['pgrep','-af','qemu-system|ninja|test_system.py|observe_phase335.py|archive_old_mesa.py|build.sh|build_media_provider.py'],capture_output=True,text=True)
if r.returncode not in (0,1):
 raise RuntimeError('Cannot inspect active jobs; refusing compact.')
if r.stdout.strip():
 print('Refusing compact: an active test/build/cleanup exists.');raise SystemExit(2)
state=json.loads((root/'docs/wip/phase335/checkpoint20/normal-build-identity.json').read_text())
assert state.get('exit')==0 and state.get('normal_restored_exact'), 'Full regression/normal restore not verified'
assert state.get('normal_uefi_verified'), 'New normal UEFI image not verified'
expected=state['final_normal_identity']
for n,want in expected.items():
 h=hashlib.sha256()
 with (root/'build'/n).open('rb') as f:
  for chunk in iter(lambda:f.read(1048576),b''):h.update(chunk)
 assert h.hexdigest()==want,n
if len(sys.argv)>1 and sys.argv[1]=='verify':
 print('Normal five hashes exact; verification alone does not prove Windows compaction ran.');raise SystemExit(0)
subprocess.run(['sync'],check=True);subprocess.run(['fstrim','-v','/'],check=True)
print('Preflight: no active test/build; normal hashes exact; trim completed.')
