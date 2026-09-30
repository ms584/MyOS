"""UEFI low-memory ownership regression: real SB16 DMA with framebuffer output.

Build the packaged image with tools/build_uefi.py before running this test.
This is the existing MYV1 regression, not Phase 33.3 codec/browser acceptance.
"""
import array
from pathlib import Path
import re
import subprocess
import wave
from test_system import VM,OUT,symbols
from test_network import send
from test_video import frame_shot

def main():
    disk=OUT/'dma-layout-uefi-data.img';disk.unlink(missing_ok=True)
    subprocess.run(['python3','tools/make_video_disk.py',str(disk)],check=True)
    capture=OUT/'dma-layout-uefi.wav';capture.unlink(missing_ok=True)
    vm=VM('dma-layout-uefi',ram=128,disk=disk,disk_index=1,
          boot_image=Path('build/myos-uefi.img'),boot_bus='ide',
          firmware=(Path('/usr/share/OVMF/OVMF_CODE_4M.fd'),Path('/usr/share/OVMF/OVMF_VARS_4M.fd')),
          audio=capture)
    try:
        vm.wait('myos> ',35);addresses=symbols()
        assert addresses['__bss_end']<0x80000
        stack=int.from_bytes(vm.memory(addresses['boot_state']+16,8),'little')
        dma=int.from_bytes(vm.memory(addresses['boot_state']+64,8),'little')
        assert stack>=65536 and not stack%4096
        assert 0x80000<=dma<=0x1000000-16384 and not dma%16384
        assert dma+16384<=stack-65536 or dma>=stack
        graphics_before=vm.memory(addresses['screen'],24)
        send(vm,'video play /home/demo.myv');vm.wait('VIDEO FRAME 0 tick',15)
        frame_shot(vm,'dma-layout-uefi-frame.ppm',[(280,260,(0xf5,0xb6,0x42))])
        vm.wait('VIDEO COMPLETE',10)
        assert vm.memory(addresses['screen'],24)==graphics_before,'DMA overwrote graphics'
        pairs=re.findall(r'VIDEO FRAME (\d+) tick (\d+)',vm.log.read_text());assert len(pairs)>=25,pairs
        assert 'PANIC' not in vm.log.read_text()
    finally:vm.close()
    with wave.open(str(capture),'rb') as sound:
        assert (sound.getnchannels(),sound.getsampwidth(),sound.getframerate())==(2,2,44100)
        pcm=array.array('h',sound.readframes(sound.getnframes()))
    assert pcm and max(abs(v) for v in pcm)>1000,'UEFI DMA produced no audible PCM'
    mono=pcm[::2];rate=44100;chunk=rate//10
    first=next(i for i in range(0,len(mono)-chunk,chunk)
               if max(abs(v) for v in mono[i:i+chunk])>1000)
    def frequency(seconds):
        at=first+int(seconds*rate);part=mono[at:at+rate//5]
        assert len(part)==rate//5,'truncated UEFI PCM timeline'
        return sum(part[i-1]<0<=part[i] for i in range(1,len(part)))*5
    first_tone,second_tone=frequency(.4),frequency(1.8)
    assert 400<=first_tone<=480 and 620<=second_tone<=700,(first_tone,second_tone)
    print('PASS: UEFI reserved kernel/BSS, separate initial stack and SB16 DMA; '
          'real video framebuffer, paced frames, graphics-state integrity and '
          f'captured PCM timeline ({first_tone}/{second_tone} Hz)')

if __name__=='__main__':main()
