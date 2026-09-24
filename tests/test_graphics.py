"""QEMU framebuffer acceptance using real PS/2 input and rendered pixels."""
from pathlib import Path
from test_system import VM,OUT
from test_shell import command,type_text
import time

def pixels(path):
    blob=Path(path).read_bytes()
    magic,dims,depth,data=blob.split(b'\n',3)
    assert magic==b'P6' and depth==b'255'
    width,height=map(int,dims.split())
    assert len(data)==width*height*3
    def sample(x,y):
        pos=(y*width+x)*3
        return tuple(data[pos:pos+3])
    return width,height,sample

def exercise(text=False):
    vm=VM('graphics-vga' if text else 'graphics-framebuffer')
    try:
        vm.wait('myos> ')
        if text:
            command(vm,'gfx','Graphics requires framebuffer mode')
            command(vm,'pwd','/\n')
            print('PASS: graphics command preserves VGA text fallback',flush=True)
            return
        type_text(vm,'gfx\n')
        vm.wait('GRAPHICS DEMO READY')
        screenshot=OUT/'graphics-framebuffer.ppm'
        vm.command('screendump',{'filename':str(screenshot)})
        width,height,sample=pixels(screenshot)
        assert (width,height)==(800,600)
        assert sample(100,20)==(0x23,0x3b,0x62),sample(100,20)
        assert sample(20,500)==(0x10,0x1a,0x2e)
        assert sample(0,350)==(0xe8,0x5d,0x75)
        assert sample(799,599)==(0x23,0xc4,0xcf)
        assert sample(100,210)==(0x23,0xc4,0xcf)
        assert sample(300,210)==(0xf5,0xb6,0x42)
        # Text pixels are rendered from the BIOS font on the off-screen surface.
        heading={sample(x,y) for y in range(28,44) for x in range(32,224)}
        assert (255,255,255) in heading and (0x23,0x3b,0x62) in heading
        vm.key('r');vm.wait('GRAPHICS REDRAW')
        vm.command('screendump',{'filename':str(screenshot)})
        _,_,sample=pixels(screenshot)
        assert sample(100,20)==(0x62,0x3b,0x72),'redraw did not update screen'
        vm.key('ret');vm.wait('Graphics demo closed\nmyos> ')
        # A second full-size surface must fit: the first one was released.
        type_text(vm,'gfx\n')
        deadline=time.monotonic()+8
        while vm.log.read_text().count('GRAPHICS DEMO READY')<2 and time.monotonic()<deadline:
            time.sleep(.03)
        assert vm.log.read_text().count('GRAPHICS DEMO READY')==2
        vm.key('ret')
        while vm.log.read_text().count('Graphics demo closed')<2 and time.monotonic()<deadline:
            time.sleep(.03)
        assert vm.log.read_text().count('Graphics demo closed')==2
        print('PASS: graphics screenshot, clipping, text, redraw and surface cleanup',flush=True)
    finally:vm.close()
