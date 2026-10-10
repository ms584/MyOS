"""Open a fresh normal MyOS/QEMU boot for owner audiovisual acceptance.
Host provides virtual hardware only; all browser/media work remains in MyOS.
No automatic input, acceptance or host media player. Requires WSLg/PulseAudio/KVM.
"""
import argparse,json,hashlib,subprocess,os
from pathlib import Path
p=argparse.ArgumentParser(description=__doc__);p.add_argument('bundle',type=Path);a=p.parse_args();b=a.bundle.resolve();root=Path.cwd()
assert b.is_relative_to((root/'build').resolve())
d=json.loads((b/'evidence.json').read_text());assert d['normal_identity']==d['normal_identity_after']
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
frozen=json.loads((b/'manual-assets-identity.json').read_text())
assert all(sha(b/name)==digest for name,digest in frozen.items())
assert sha(b/'boot.img')==d['normal_identity']['myos-current.img']
assert sha(root/'build/platform-runtime.mrt')==d['normal_identity']['platform-runtime.mrt']
assert sha(root/'build/engine-evaluation/libcobalt.so')==d['core_sha256']
assert os.geteuid()!=0,'Run as the normal WSL user'
assert os.access('/dev/kvm',os.R_OK|os.W_OK),'Non-root KVM access required'
command=['qemu-system-x86_64','-m','1024','-accel','kvm','-cpu','host','-display','gtk','-monitor','none','-serial',f'file:{b}/manual-serial.log','-no-reboot','-no-shutdown','-drive',f'format=raw,file={b}/boot.img,if=floppy,index=0','-boot','order=a','-nic','user,model=rtl8139','-audiodev','pa,id=myos-audio,out.frequency=11025,out.channels=1,out.format=u8','-device','sb16,audiodev=myos-audio','-drive',f'format=raw,file={b}/assets.img,if=ide,index=1,readonly=on']
print('Fresh native MyOS manual trial; no phase acceptance is recorded automatically.',flush=True)
print('In the MyOS shell: net up; mkdir /dev/shm; mem',flush=True)
print('Then type: elf cobalt cobalt provider https://www.youtube.com/watch?v=dQw4w9WgXcQ',flush=True)
print('Select native YouTube Quality144p. Watch/listen>=30s; K pause/resume; L forward10s; J back10s; Esc close; mem; desktop,open a window,Q; reopen. Repeat after fresh boot.',flush=True)
raise SystemExit(subprocess.call(command))
