"""Phase 15 QEMU PS/2 mouse and concurrent keyboard acceptance."""
import time
from test_system import VM,symbols
from test_shell import command,type_text
from test_graphics import pixels
from pathlib import Path

def send_relative(vm,axis,value):
    vm.command('input-send-event',{'events':[{'type':'rel','data':{'axis':axis,'value':value}}]})

def send_button(vm,down):
    vm.command('input-send-event',{'events':[{'type':'btn','data':{'down':down,'button':'left'}}]})

def exercise(text=False):
    vm=VM('mouse-vga' if text else 'mouse-framebuffer')
    try:
        output=vm.wait('myos> ')
        assert 'PS/2 mouse initialized' in output,output
        assert 'Mouse: x=' in command(vm,'mouse')
        if text:
            command(vm,'gfx','Graphics requires framebuffer mode')
            send_relative(vm,'x',40)
            command(vm,'mouse','Mouse: x=360 y=200 buttons=0')
            print('PASS: PS/2 mouse remains available in VGA text fallback',flush=True)
            return
        type_text(vm,'gfx\n');vm.wait('GRAPHICS DEMO READY')
        picture=Path('build/system-tests/mouse-framebuffer.ppm')
        vm.command('screendump',{'filename':str(picture)})
        width,height,sample=pixels(picture)
        assert (width,height)==(800,600)
        assert sample(400,300)==(0x10,0x10,0x10)
        start=int.from_bytes(vm.memory(symbols()['ticks'],8),'little')
        send_relative(vm,'x',40);send_relative(vm,'y',-20)
        deadline=time.monotonic()+5
        while vm.log.read_text().count('MOUSE EVENT')<2 and time.monotonic()<deadline:
            time.sleep(.03)
        assert vm.log.read_text().count('MOUSE EVENT')>=2
        vm.command('screendump',{'filename':str(picture)})
        _,_,sample=pixels(picture)
        assert sample(400,300)==(0x1c,0x2b,0x43),sample(400,300)
        assert sample(440,280)==(0x10,0x10,0x10),sample(440,280)
        send_button(vm,True);vm.wait('MOUSE LEFT DOWN')
        vm.command('screendump',{'filename':str(picture)})
        _,_,sample=pixels(picture)
        assert sample(441,282)==(0xf5,0xb6,0x42),sample(441,282)
        vm.key('r');vm.wait('GRAPHICS REDRAW')
        vm.command('screendump',{'filename':str(picture)})
        _,_,sample=pixels(picture)
        assert sample(441,282)==(0xf5,0xb6,0x42)
        send_button(vm,False)
        vm.key('ret');vm.wait('Graphics demo closed\nmyos> ')
        end=int.from_bytes(vm.memory(symbols()['ticks'],8),'little')
        assert end>start+5,(start,end)
        command(vm,'mouse','Mouse: x=440 y=280 buttons=0')
        # The Shell does not consume mouse events. A burst must remain bounded.
        for i in range(90):send_relative(vm,'x',1 if i%2 else -1)
        deadline=time.monotonic()+5
        while time.monotonic()<deadline:
            result=command(vm,'mouse','Mouse: x=')
            number=int(result.split('dropped=')[1].split()[0])
            if number>0:break
            time.sleep(.05)
        assert number>0,result
        for _ in range(8):
            send_relative(vm,'x',-120);send_relative(vm,'y',-100)
        command(vm,'mouse','Mouse: x=0 y=0 buttons=0')
        for _ in range(8):
            send_relative(vm,'x',120);send_relative(vm,'y',100)
        command(vm,'mouse','Mouse: x=799 y=599 buttons=0')
        command(vm,'pwd','/\n')
        print('PASS: PS/2 movement, click, bounded cursor, keyboard/redraw, timer, bounded queue and Shell',flush=True)
    finally:vm.close()
