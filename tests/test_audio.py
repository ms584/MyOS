"""Phase 30 SB16 DMA and QEMU WAV-output acceptance."""
import array,subprocess,sys,time,wave
from pathlib import Path
sys.path.insert(0,'tests')
from test_system import VM,OUT,symbols
from test_network import send,wait_count

def main():
    bare=VM('phase30-no-audio')
    try:
        bare.wait('myos> ',30)
        send(bare,'audio status')
        bare.wait('Audio: no SB16 device')
        send(bare,'audio tone 440 250')
        bare.wait('Audio unavailable, invalid tone or queue full')
    finally:bare.close()
    path=OUT/'phase30-capture.wav'
    path.unlink(missing_ok=True)
    vm=VM('phase30-sb16',audio=path)
    try:
        vm.wait('Audio: SB16 initialized',60)
        vm.wait('myos> ',60)
        addresses=symbols();graphics_before=vm.memory(addresses['screen'],24)
        assert addresses['__bss_end']<=0x80000
        assert int.from_bytes(vm.memory(addresses['boot_state']+16,8),'little')==0x90000
        send(vm,'audio tone 440 300')
        vm.wait('Audio tone queued')
        time.sleep(3)
        assert vm.memory(addresses['screen'],24)==graphics_before,'SB16 DMA overwrote graphics state'
        send(vm,'audio status')
        vm.wait('underruns=')
        assert 'underruns=0\n' not in vm.log.read_text().split('myos> audio status')[-1]
        send(vm,'audio tone 660 300')
        wait_count(vm,'Audio tone queued',2)
        time.sleep(3)
        send(vm,'audio stop')
        vm.wait('Audio stopped')
        send(vm,'audio volume 25')
        vm.wait('volume=25')
        send(vm,'audio tone 440 250')
        wait_count(vm,'Audio tone queued',3)
        time.sleep(2)
        send(vm,'audio stop')
        wait_count(vm,'Audio stopped',2)
        send(vm,'audio mute on')
        vm.wait('muted')
        send(vm,'audio tone 440 250')
        wait_count(vm,'Audio tone queued',4)
        time.sleep(2)
        send(vm,'audio stop')
        wait_count(vm,'Audio stopped',3)
        send(vm,'audio mute off')
        vm.wait('unmuted')
        assert 'PANIC' not in vm.log.read_text()
    finally:vm.close()
    assert path.is_file() and path.stat().st_size>1024,path
    with wave.open(str(path),'rb') as sound:
        channels,width,rate,frames=sound.getnchannels(),sound.getsampwidth(),sound.getframerate(),sound.getnframes()
        content=sound.readframes(frames)
    assert frames>rate//2 and content,(channels,width,rate,frames)
    samples=array.array('h',content)
    assert width==2 and channels==2 and max(abs(x) for x in samples)>1000,'no audible PCM in WAV capture'
    assert max(abs(x) for x in samples[-rate*channels:])<1000,'mute/stop did not leave a silent tail'
    chunk=rate*channels//2
    peaks=[max(abs(x) for x in samples[i:i+chunk]) for i in range(0,len(samples)-chunk,chunk)]
    bursts=[p for p in peaks if p>1000]
    assert len(bursts)>=3 and min(bursts)<max(bursts)//2,peaks
    assert any(p<1000 for p in peaks[1:-2]),peaks
    print(f'PASS: SB16 DMA output, WAV capture ({channels}ch {width*8}bit {rate}Hz, {frames} frames), underrun/recovery, volume and mute controls')

    source=OUT/'phase30-source.wav'
    source.unlink(missing_ok=True)
    with wave.open(str(source),'wb') as sound:
        sound.setnchannels(1);sound.setsampwidth(1);sound.setframerate(11025)
        sound.writeframes(bytes(208 if (i//13)%2 else 48 for i in range(11025)))
    disk=OUT/'phase30-data.img'
    disk.unlink(missing_ok=True)
    subprocess.run(['python3','tools/make_audio_disk.py',str(disk),str(source)],check=True)
    saved=OUT/'phase30-file-capture.wav'
    saved.unlink(missing_ok=True)
    vm=VM('phase30-file',disk=disk,audio=saved)
    try:
        # Process self-tests plus v2 ATA import measured 30.74s on WSL.
        # Use the common bounded 60s shell-boot budget, not the media timeout.
        vm.wait('myos> ')
        send(vm,'audio play /etc/motd')
        vm.wait('Audio unavailable, invalid WAV or queue full')
        send(vm,'audio play /home/sample.wav')
        vm.wait('Audio WAV queued')
        time.sleep(3)
        send(vm,'audio stop')
        vm.wait('Audio stopped')
    finally:vm.close()
    with wave.open(str(saved),'rb') as sound:
        pcm=array.array('h',sound.readframes(sound.getnframes()))
    assert max(abs(x) for x in pcm)>1000,'imported WAV was silent'
    print('PASS: imported 11025 Hz mono 8-bit WAV plays from MyOS v2 filesystem; invalid file rejected')

if __name__=='__main__':main()
