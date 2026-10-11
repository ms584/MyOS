"""Open a fresh normal MyOS/QEMU boot for owner audiovisual acceptance.
Host provides virtual hardware only; all browser/media work remains in MyOS.
No automatic input, acceptance or host media player. Requires WSLg/PulseAudio/KVM.
"""
import argparse,json,hashlib,subprocess,os,tempfile,time
from pathlib import Path
p=argparse.ArgumentParser(description=__doc__);p.add_argument('bundle',type=Path);p.add_argument('--check-boot',action='store_true',help='Passive headless boot check using the same drives/PulseAudio; no browser input or acceptance');a=p.parse_args();b=a.bundle.resolve();root=Path.cwd()
assert b.is_relative_to((root/'build').resolve())
d=json.loads((b/'evidence.json').read_text());assert d['normal_identity']==d['normal_identity_after']
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
frozen=json.loads((b/'manual-assets-identity.json').read_text())
assert all(sha(b/name)==digest for name,digest in frozen.items())
assert sha(b/'boot.img')==d['normal_identity']['myos-current.img']
assert sha(root/'build/platform-runtime.mrt')==d['normal_identity']['platform-runtime.mrt']
official=json.loads((root/'docs/PHASE33_COBALT_AUDIT.json').read_text())
assert sha(root/'build/engine-evaluation/libcobalt.so')==official['library_sha256']
source_core=d.get('source_core_build')
if source_core is None:
    assert sha(root/'build/engine-evaluation/libcobalt.so')==d['core_sha256']
else:
    from cobalt_source_core import load_source_core
    core,validated=load_source_core(root/'build/phase335-core-workspace/core-build-identity.json')
    assert validated==source_core,'Manual bundle source-core identity mismatch'
    assert d.get('official_google_core') is False
    assert sha(core)==d['core_sha256']

assert os.geteuid()!=0,'Run as the normal WSL user'
assert os.access('/dev/kvm',os.R_OK|os.W_OK),'Non-root KVM access required'
session=Path(tempfile.mkdtemp(prefix='manual-session-',dir=b));serial=session/'serial.log'
command=['qemu-system-x86_64','-m','1024','-accel','kvm','-cpu','host','-display','none' if a.check_boot else 'gtk','-monitor','none','-serial',f'file:{serial}','-no-reboot','-no-shutdown','-drive',f'format=raw,file={b}/boot.img,if=floppy,index=0,snapshot=on','-boot','order=a','-nic','user,model=rtl8139','-audiodev','pa,id=myos-audio,out.frequency=11025,out.channels=1,out.format=u8','-device','sb16,audiodev=myos-audio','-drive',f'format=raw,file={b}/assets.img,if=ide,index=1,snapshot=on']
(session/'qemu-command.json').write_text(json.dumps(dict(command=command,boot_check=a.check_boot,phase_passed=False,user_acceptance_received=False,source_bundle=str(b),source_core_build=source_core,frozen_inputs=frozen),indent=2)+'\n')
print('Serial log:',serial,flush=True)
print('Fresh native MyOS manual trial; no phase acceptance is recorded automatically.',flush=True)
print('In the MyOS shell: net up; mkdir /dev/shm; mem',flush=True)
print('Then type: elf cobalt cobalt provider https://www.youtube.com/watch?v=dQw4w9WgXcQ',flush=True)
print('Select native YouTube Quality144p. Watch/listen>=30s; K pause/resume; L forward10s; J back10s; Esc close; mem; desktop,open a window,Q; reopen. Repeat after fresh boot.',flush=True)
result=1
try:
    with subprocess.Popen(command) as process:
        try:
            if a.check_boot:
                deadline=time.monotonic()+30
                while time.monotonic()<deadline:
                    if serial.exists() and 'myos> ' in serial.read_text():
                        print('PASS passive normal MyOS boot; writable temporary drive overlays, immutable source images. Playback/speaker acceptance not tested.',flush=True)
                        result=0;break
                    if process.poll() is not None:
                        print('FAIL: QEMU exited before MyOS shell, exit',process.returncode,flush=True);break
                    time.sleep(.1)
                else:print('FAIL: MyOS shell not observed within30s',flush=True)
            else:result=process.wait()
        finally:
            if process.poll() is None:
                process.terminate()
                try:process.wait(timeout=5)
                except subprocess.TimeoutExpired:process.kill();process.wait()
finally:
    assert all(sha(b/name)==digest for name,digest in frozen.items()),'Frozen manual source inputs changed'
raise SystemExit(result)
