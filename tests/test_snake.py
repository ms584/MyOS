"""Phase 19 Snake acceptance with QEMU PIT, PS/2 input and ATA snapshots."""
import sys
import time
from test_system import VM,OUT,ROOT,symbols
from test_shell import command,type_text
from test_desktop import move_to,button,wait_count,shot
sys.path.insert(0,str(ROOT))
from tools.create_data_disk import create_image

def click(vm,addresses,x,y,marker):
    move_to(vm,addresses,x,y)
    count=vm.log.read_text().count(marker)
    button(vm,True)
    wait_count(vm,marker,count+1)
    button(vm,False)

def open_snake(vm):
    type_text(vm,'desktop\n');vm.wait('DESKTOP READY')
    vm.key('6');vm.wait('WINDOW OPEN 6')

def leave(vm):
    vm.key('q');vm.wait('DESKTOP CLOSED\nmyos> ')

def exercise(text=False):
    if text:
        vm=VM('snake-vga')
        try:
            vm.wait('myos> ')
            command(vm,'desktop','Desktop requires framebuffer >= 640x480')
            command(vm,'pwd','/\n')
            print('PASS: Snake VGA text fallback leaves Shell usable',flush=True)
        finally:vm.close()
        return

    vm=VM('snake-ram')
    try:
        vm.wait('myos> ');open_snake(vm)
        addresses=symbols()
        vm.key('p');vm.wait('SNAKE PAUSE 0')
        steps=vm.log.read_text().count('SNAKE STEP')
        time.sleep(.4)
        assert vm.log.read_text().count('SNAKE STEP')==steps,'paused game advanced'
        vm.key('up');vm.wait('SNAKE TURN 3')
        vm.key('left') # A second turn before a step must not bypass reversal.
        vm.key('p');vm.wait('SNAKE RESUME 0')
        vm.wait('SNAKE STEP 45',timeout=4) # (5,2), proving arrow dispatch.
        vm.key('p');vm.wait('SNAKE PAUSE 0')
        vm.key('r');vm.wait('SNAKE RESTART 0')
        vm.wait('SNAKE SCORE 1',timeout=4)
        vm.wait('SNAKE HIGH RAM 1')
        vm.key('p');vm.wait('SNAKE PAUSE 1')
        pixel=shot(vm,'snake-framebuffer.ppm')
        board={pixel(x,y) for y in range(414,526) for x in range(500,780)}
        assert (0x92,0xe4,0x6b) in board,'snake head is missing'
        assert (0xf0,0xa3,0x4e) in board,'food is missing'
        vm.key('p');vm.wait('SNAKE RESUME 1')
        before=vm.log.read_text().count('SNAKE STEP')
        vm.key('5');vm.wait('WINDOW OPEN 5')
        for key in ('2',):vm.key(key)
        vm.key('shift','equal')
        vm.key('2');vm.key('equal');vm.wait('CALC: 4\n')
        deadline=time.monotonic()+3
        while vm.log.read_text().count('SNAKE STEP')<=before and time.monotonic()<deadline:
            time.sleep(.02)
        assert vm.log.read_text().count('SNAKE STEP')>before,'Snake stopped behind Calculator'
        vm.wait('SNAKE GAME OVER',timeout=10)
        # Exposed right edge of Snake is outside the overlapping Calculator.
        move_to(vm,addresses,760,450)
        focused=vm.log.read_text().count('WINDOW FOCUS 6')
        button(vm,True);wait_count(vm,'WINDOW FOCUS 6',focused+1);button(vm,False)
        restarted=vm.log.read_text().count('SNAKE RESTART 0')
        vm.key('r');wait_count(vm,'SNAKE RESTART 0',restarted+1)
        click(vm,addresses,550,545,'SNAKE PAUSE 0')
        click(vm,addresses,720,545,'SNAKE RESTART 0')
        leave(vm)
        command(vm,'stat /home/snake.score','file 12 bytes')
        print('PASS: Snake arrows, pause/restart, timer, growth, collision, pixels and concurrent Calculator',flush=True)
    finally:vm.close()
    vm=VM('snake-ram-reboot')
    try:
        vm.wait('myos> ');open_snake(vm)
        vm.wait('SNAKE START 0')
        leave(vm)
        print('PASS: RAM-only high score resets on reboot',flush=True)
    finally:vm.close()

    disk=OUT/'phase19-data.img'
    disk.unlink(missing_ok=True) # Disposable test fixture under ignored build/.
    create_image(disk)
    vm=VM('snake-disk-first',disk=disk)
    try:
        vm.wait('myos> ');open_snake(vm)
        vm.wait('SNAKE SCORE 1',timeout=4)
        vm.wait('SNAKE HIGH SAVED 1',timeout=5)
        leave(vm)
        command(vm,'stat /home/snake.score','file 12 bytes')
        print('PASS: high score saved through Phase 13 ATA snapshot',flush=True)
    finally:vm.close()
    vm=VM('snake-disk-reboot',disk=disk)
    try:
        output=vm.wait('myos> ')
        assert 'SNAKE HIGH RESTORED 1' in output,output
        open_snake(vm);vm.wait('SNAKE START 1')
        leave(vm)
        command(vm,'write /home/snake.score BAD')
    finally:vm.close()
    vm=VM('snake-invalid-file',disk=disk)
    try:
        output=vm.wait('myos> ')
        assert 'SNAKE HIGH INVALID 0' in output,output
        open_snake(vm);vm.wait('SNAKE START 0')
        leave(vm)
        print('PASS: saved high score survives reboot; invalid record safely rejected',flush=True)
    finally:vm.close()

def low_memory():
    vm=VM('snake-16M',ram=16)
    try:
        vm.wait('myos> ');open_snake(vm)
        vm.wait('SNAKE STEP',timeout=4)
        leave(vm)
        print('PASS: Snake runs at 16 MiB RAM',flush=True)
    finally:vm.close()
