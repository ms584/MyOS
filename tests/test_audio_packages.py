"""Smoke test SB16 audio in packaged BIOS and UEFI images."""
import array,sys,time,wave
from pathlib import Path
sys.path.insert(0,'tests')
from test_system import VM,OUT
from test_network import send

def main():
    for firmware in ('bios','uefi'):
        path=OUT/f'phase30-{firmware}-capture.wav'
        path.unlink(missing_ok=True)
        fw=None
        if firmware=='uefi':
            fw=(Path('/usr/share/OVMF/OVMF_CODE_4M.fd'),Path('/usr/share/OVMF/OVMF_VARS_4M.fd'))
        vm=VM(f'phase30-{firmware}',ram=128 if fw else 16,
              boot_image=Path(f'build/myos-{firmware}.img'),boot_bus='ide',
              firmware=fw,audio=path)
        try:
            vm.wait('Audio: SB16 initialized',40)
            vm.wait('myos> ',40)
            send(vm,'audio tone 440 500')
            vm.wait('Audio tone queued',10)
            time.sleep(3)
            send(vm,'audio status')
            vm.wait('IRQs=')
        finally:vm.close()
        with wave.open(str(path),'rb') as sound:
            pcm=array.array('h',sound.readframes(sound.getnframes()))
        assert pcm and max(abs(x) for x in pcm)>1000,firmware
        print(f'PASS: packaged {firmware.upper()} SB16 audio output',flush=True)
if __name__=='__main__':main()
