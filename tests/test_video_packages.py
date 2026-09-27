"""Phase 31 local video playback from packaged BIOS and UEFI images."""
import array
import subprocess
import sys
import wave
from pathlib import Path

sys.path.insert(0,'tests')
from test_system import VM,OUT
from test_network import send

def main():
    disk=OUT/'phase31-package-data.img'
    disk.unlink(missing_ok=True)
    subprocess.run(['python3','tools/make_video_disk.py',str(disk)],check=True)
    for target in ('bios','uefi'):
        capture=OUT/f'phase31-{target}-capture.wav'
        capture.unlink(missing_ok=True)
        firmware=None
        if target=='uefi':
            firmware=(Path('/usr/share/OVMF/OVMF_CODE_4M.fd'),
                      Path('/usr/share/OVMF/OVMF_VARS_4M.fd'))
        vm=VM(f'phase31-{target}-package',ram=128 if firmware else 64,
              disk=disk,disk_index=1,
              boot_image=Path(f'build/myos-{target}.img'),
              boot_bus='ide',
              firmware=firmware,audio=capture)
        try:
            vm.wait('myos> ',40)
            send(vm,'video play /home/demo.myv')
            vm.wait('VIDEO COMPLETE',15)
            vm.wait('Video complete: displayed',5)
            assert 'PANIC' not in vm.log.read_text()
        finally:
            vm.close()
        with wave.open(str(capture),'rb') as sound:
            samples=array.array('h',sound.readframes(sound.getnframes()))
        assert samples and max(abs(x) for x in samples)>1000,target
        print(f'PASS: packaged {target.upper()} video decoded and SB16 PCM captured',flush=True)

if __name__=='__main__':main()
