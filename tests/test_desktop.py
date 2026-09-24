"""Phase 16 QEMU desktop/window acceptance using real PS/2 input."""
from pathlib import Path
import time
from test_system import VM,OUT,symbols
from test_shell import command,type_text
from test_graphics import pixels

def send(vm,events):
    vm.command('input-send-event',{'events':events})

def relative(vm,x,y):
    events=[]
    for axis,value in (('x',x),('y',y)):
        if value:events.append({'type':'rel','data':{'axis':axis,'value':value}})
    if events:send(vm,events)

def button(vm,down):
    send(vm,[{'type':'btn','data':{'down':down,'button':'left'}}])

def position(vm,addresses):
    return tuple(int.from_bytes(vm.memory(addresses[name],4),'little',signed=True)
                 for name in ('x_position','y_position'))

def move_to(vm,addresses,x,y):
    for _ in range(25):
        current_x,current_y=position(vm,addresses)
        if (current_x,current_y)==(x,y):return
        dx=max(-100,min(100,x-current_x))
        dy=max(-100,min(100,y-current_y))
        relative(vm,dx,dy)
        expected=(current_x+dx,current_y+dy)
        deadline=time.monotonic()+3
        while position(vm,addresses)!=expected and time.monotonic()<deadline:
            time.sleep(.01)
        assert position(vm,addresses)==expected,(x,y,expected,position(vm,addresses))
    raise AssertionError(('mouse could not reach',x,y,position(vm,addresses)))

def wait_count(vm,marker,count):
    deadline=time.monotonic()+5
    while vm.log.read_text().count(marker)<count and time.monotonic()<deadline:
        time.sleep(.02)
    assert vm.log.read_text().count(marker)>=count,(marker,count,vm.log.read_text())

def shot(vm,name):
    path=OUT/name
    vm.command('screendump',{'filename':str(path)})
    width,height,pixel=pixels(path)
    assert (width,height)==(800,600)
    return pixel

def exercise(text=False):
    if text:
        vm=VM('desktop-vga')
        try:
            vm.wait('myos> ')
            command(vm,'desktop','Desktop requires framebuffer >= 640x480')
            command(vm,'pwd','/\n')
            print('PASS: desktop preserves VGA text Shell fallback',flush=True)
        finally:vm.close()
        return
    vm=VM('desktop-framebuffer')
    try:
        vm.wait('myos> ')
        command(vm,'desktop extra','error: unexpected argument')
        type_text(vm,'desktop\n');vm.wait('DESKTOP READY')
        addresses=symbols()
        before=int.from_bytes(vm.memory(addresses['ticks'],8),'little')
        pixel=shot(vm,'desktop-empty.ppm')
        assert pixel(200,300)==(0x11,0x1a,0x2c)
        assert pixel(30,100)==(0x2f,0x47,0x68)
        frames=vm.log.read_text().count('DESKTOP FRAME')
        move_to(vm,addresses,80,105);button(vm,True);wait_count(vm,'WINDOW OPEN 1',1);button(vm,False)
        frames+=1;wait_count(vm,'DESKTOP FRAME',frames)
        pixel=shot(vm,'desktop-one.ppm')
        assert pixel(30,100)==(0x2c,0x66,0x72)
        move_to(vm,addresses,80,160);button(vm,True);wait_count(vm,'WINDOW OPEN 2',1);button(vm,False)
        frames+=1;wait_count(vm,'DESKTOP FRAME',frames)
        pixel=shot(vm,'desktop-framebuffer.ppm')
        assert pixel(540,230)==(0x3a,0x2b,0x43),pixel(540,230)
        move_to(vm,addresses,245,145);button(vm,True);wait_count(vm,'WINDOW FOCUS 1',1);button(vm,False)
        frames+=1;wait_count(vm,'DESKTOP FRAME',frames)
        pixel=shot(vm,'desktop-focused.ppm')
        assert pixel(540,230)==(0x20,0x32,0x4b),pixel(540,230)
        button(vm,True)
        move_to(vm,addresses,305,175)
        wait_count(vm,'WINDOW DRAG 1',1)
        button(vm,False)
        time.sleep(.1)
        pixel=shot(vm,'desktop-dragged.ppm')
        assert pixel(232,132)==(0x11,0x1a,0x2c),pixel(232,132)
        assert pixel(300,180)==(0x27,0x6b,0x8a),pixel(300,180)
        move_to(vm,addresses,590,175);button(vm,True);wait_count(vm,'WINDOW CLOSE 1',1);button(vm,False)
        time.sleep(.05)
        pixel=shot(vm,'desktop-close.ppm')
        assert pixel(540,230)==(0x3a,0x2b,0x43),pixel(540,230)
        move_to(vm,addresses,580,195);button(vm,True);wait_count(vm,'WINDOW CLOSE 2',1);button(vm,False)
        vm.key('3');wait_count(vm,'WINDOW OPEN 3',1)
        move_to(vm,addresses,630,245);button(vm,True);wait_count(vm,'WINDOW CLOSE 3',1);button(vm,False)
        move_to(vm,addresses,530,145)
        for cycle in range(8):
            vm.key('1');wait_count(vm,'WINDOW OPEN 1',cycle+2)
            button(vm,True);wait_count(vm,'WINDOW CLOSE 1',cycle+2);button(vm,False)
        after=int.from_bytes(vm.memory(addresses['ticks'],8),'little')
        assert after>before+20,(before,after)
        vm.key('q');vm.wait('DESKTOP CLOSED\nmyos> ')
        for cycle in range(4):
            type_text(vm,'desktop\n');wait_count(vm,'DESKTOP READY',cycle+2)
            vm.key('esc');wait_count(vm,'DESKTOP CLOSED',cycle+2)
        command(vm,'pwd','/\n')
        type_text(vm,'gfx\n');vm.wait('GRAPHICS DEMO READY');vm.key('ret')
        vm.wait('Graphics demo closed\nmyos> ')
        print('PASS: launcher, focus/stacking, drag, close, redraw, repeated release, timer and Shell',flush=True)
    finally:vm.close()

def low_memory():
    vm=VM('desktop-16M',ram=16)
    try:
        vm.wait('myos> ')
        type_text(vm,'desktop\n');vm.wait('DESKTOP READY')
        vm.key('1');vm.wait('WINDOW OPEN 1')
        vm.key('q');vm.wait('DESKTOP CLOSED\nmyos> ')
        print('PASS: desktop and window at 16 MiB RAM',flush=True)
    finally:vm.close()
