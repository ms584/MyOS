"""Phase 31 native MYV1 video decode, framebuffer timing and PCM sync acceptance."""
import array,re,struct,subprocess,sys,time,wave
from pathlib import Path
sys.path.insert(0,'tests')
from test_system import VM,OUT,symbols
from test_network import send,wait_count
from test_graphics import pixels

DISK=OUT/'phase31-data.img'
def shot(vm,name):
    path=OUT/name
    vm.command('screendump',{'filename':str(path)})
    width,height,pixel=pixels(path)
    assert (width,height)==(800,600)
    return pixel

def frame_shot(vm,name,expected):
    # A serial present marker and QEMU's display surface update are different
    # events. Freeze the guest so a display refresh cannot be mistaken for a
    # later video frame; first prove the actual guest framebuffer is correct.
    vm.command('stop')
    try:
        info=vm.memory(symbols()['boot_state'],64)
        width,height,pitch,framebuffer=struct.unpack_from('<IIIQ',info,44)
        assert (width,height)==(800,600)
        for x,y,color in expected:
            value=int.from_bytes(vm.memory(framebuffer+y*pitch+x*4,4),'little')
            actual=((value>>16)&255,(value>>8)&255,value&255)
            assert actual==color,('guest framebuffer',x,y,actual,color)
        end=time.monotonic()+.5
        while True:
            pixel=shot(vm,name)
            if all(pixel(x,y)==color for x,y,color in expected):return pixel
            assert time.monotonic()<end,('QEMU display refresh',[(x,y,pixel(x,y),color) for x,y,color in expected])
            time.sleep(.01)
    finally:vm.command('cont')

def main():
    DISK.unlink(missing_ok=True)
    subprocess.run(['python3','tools/make_video_disk.py',str(DISK)],check=True)
    bare=VM('phase31-no-sb16',disk=DISK)
    try:
        bare.wait('myos> ')
        send(bare,'video play /home/demo.myv')
        bare.wait('Video requires SB16 audio',8)
    finally:bare.close()
    capture=OUT/'phase31-video-capture.wav'
    capture.unlink(missing_ok=True)
    vm=VM('phase31-video',disk=DISK,audio=capture)
    try:
        vm.wait('myos> ')
        send(vm,'video play /etc/motd')
        vm.wait('Video: invalid or unavailable MYV1 clip',8)
        send(vm,'video play /home/demo.myv')
        vm.wait('VIDEO FRAME 0 tick',12)
        initial=frame_shot(vm,'phase31-initial.ppm',[(280,260,(0xf5,0xb6,0x42))])
        assert initial(280,260)==(0xf5,0xb6,0x42),initial(280,260)
        vm.wait('VIDEO FRAME 15 tick',8)
        moved=frame_shot(vm,'phase31-moved.ppm',[(280,260,(0x10,0x20,0x38)),(388,260,(0xf5,0xb6,0x42))])
        assert moved(280,260)==(0x10,0x20,0x38),moved(280,260)
        assert moved(388,260)==(0xf5,0xb6,0x42),moved(388,260)
        vm.wait('VIDEO COMPLETE',8)
        vm.wait('Video complete: displayed',5)
        vm.wait('myos> ',5)
        log=vm.log.read_text()
        start=int(re.search(r'VIDEO START tick (\d+)',log).group(1))
        pairs=[(int(a),int(b)) for a,b in re.findall(r'VIDEO FRAME (\d+) tick (\d+)',log)]
        assert len(pairs)>=25,pairs
        assert pairs[0][0]==0 and pairs[-1][0]>=28,pairs
        assert all(0<=tick-start-index*10<=10 for index,tick in pairs),pairs
        print(f'PASS: {len(pairs)} decoded video frames paced by PIT within 10 ticks; moving framebuffer pixels',flush=True)
        send(vm,'video play /home/demo.myv')
        wait_count(vm,'VIDEO FRAME 0 tick',2,12)
        vm.key('spc');vm.wait('VIDEO PAUSED',5)
        paused=shot(vm,'phase31-paused.ppm')
        frozen=paused(280,260)
        time.sleep(.4)
        paused_again=shot(vm,'phase31-paused-again.ppm')
        assert paused_again(280,260)==frozen
        vm.key('spc');vm.wait('VIDEO RESUMED',5)
        wait_count(vm,'VIDEO COMPLETE',2,8)
        send(vm,'video play /home/demo.myv')
        wait_count(vm,'VIDEO FRAME 0 tick',3,12)
        vm.key('spc');vm.wait('VIDEO PAUSED',5)
        vm.key('right');vm.wait('VIDEO SEEK frame ',5)
        seeked=shot(vm,'phase33-seek-right.ppm')
        assert seeked(280,260)==(0x10,0x20,0x38)
        vm.key('left');wait_count(vm,'VIDEO SEEK frame ',2,5)
        rewound=shot(vm,'phase33-seek-left.ppm')
        assert rewound(280,260)==(0xf5,0xb6,0x42)
        print('PASS: paused Left/Right seek redraws the target MYV1 frame',flush=True)
        vm.key('spc');wait_count(vm,'VIDEO RESUMED',2,5)
        vm.key('esc');vm.wait('VIDEO STOPPED',5)
        assert 'PANIC' not in vm.log.read_text()
        print('PASS: Space freezes/resumes video and audio; Esc stops and returns to Shell',flush=True)
    finally:vm.close()
    with wave.open(str(capture),'rb') as stream:
        assert (stream.getnchannels(),stream.getsampwidth(),stream.getframerate())==(2,2,44100)
        data=stream.readframes(stream.getnframes())
    samples=array.array('h',data)
    assert max(abs(v) for v in samples)>1000
    mono=samples[::2]
    rate=44100
    chunk=rate//10
    first=next(i for i in range(0,len(mono)-chunk,chunk)
               if max(abs(v) for v in mono[i:i+chunk])>1000)
    def frequency(seconds):
        at=first+int(seconds*rate)
        part=mono[at:at+rate//5]
        return sum(part[i-1]<0<=part[i] for i in range(1,len(part)))*5
    first_tone,second_tone=frequency(.4),frequency(1.8)
    assert 400<=first_tone<=480 and 620<=second_tone<=700,(first_tone,second_tone)
    print(f'PASS: real QEMU SB16 output follows MYV1 PCM timeline ({first_tone}/{second_tone} Hz)',flush=True)

    low=VM('phase31-video-16m',ram=16,disk=DISK,audio=OUT/'phase31-16m.wav')
    try:
        low.wait('myos> ')
        send(low,'video play /home/demo.myv')
        low.wait('Video: invalid or unavailable MYV1 clip',8)
        low.wait('myos> ',5)
        assert 'PANIC' not in low.log.read_text()
        print('PASS: 16 MiB RAM rejects unmounted large media disk without crashing',flush=True)
    finally:low.close()

    minimum=VM('phase31-video-32m',ram=32,disk=DISK,audio=OUT/'phase31-32m.wav')
    try:
        minimum.wait('myos> ')
        send(minimum,'video play /home/demo.myv')
        minimum.wait('VIDEO COMPLETE',12)
        minimum.wait('myos> ',5)
        print('PASS: 32 MiB RAM video playback and Shell recovery',flush=True)
    finally:minimum.close()

    bad=bytearray(Path('build/phase31-demo.myv').read_bytes())
    bad[24+4*3+30*2]=0  # zero RLE run is invalid
    bad_clip=OUT/'phase31-bad.myv';bad_clip.write_bytes(bad)
    bad_disk=OUT/'phase31-bad.img';bad_disk.unlink(missing_ok=True)
    subprocess.run(['python3','tools/make_video_disk.py',str(bad_disk),'--input',str(bad_clip)],check=True)
    broken=VM('phase31-video-bad',disk=bad_disk,audio=OUT/'phase31-bad.wav')
    try:
        broken.wait('myos> ')
        send(broken,'video play /home/demo.myv')
        broken.wait('Video: invalid or unavailable MYV1 clip',8)
        broken.wait('myos> ',5)
        assert 'PANIC' not in broken.log.read_text()
        print('PASS: malformed RLE clip rejected and Shell survives',flush=True)
    finally:broken.close()

if __name__=='__main__':main()
