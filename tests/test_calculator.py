"""Phase 18 Calculator acceptance through real QEMU PS/2 input and pixels."""
import time
from test_system import VM,symbols
from test_shell import command,type_text
from test_desktop import move_to,button,wait_count,shot

def key(vm,character):
    special={'*':('shift','8'),'+':('shift','equal'),
             '/':('slash',),'-':('minus',),'.':('dot',),
             '=':('equal',),'\b':('backspace',)}
    vm.key(*special.get(character,(character,)))

def press(vm,character,expected=None):
    marker='CALC: '+expected+'\n' if expected is not None else None
    rendered=marker+'DESKTOP FRAME\n' if marker else None
    count=vm.log.read_text().count(rendered) if rendered else 0
    key(vm,character)
    if rendered:wait_count(vm,rendered,count+1)

def click(vm,addresses,x,y,expected):
    move_to(vm,addresses,x,y)
    marker='CALC: '+expected+'\n'
    count=vm.log.read_text().count(marker)
    button(vm,True)
    wait_count(vm,marker,count+1)
    button(vm,False)

def sequence(vm,characters,result):
    for character in characters[:-1]:press(vm,character)
    press(vm,characters[-1],result)

def exercise(text=False):
    vm=VM('calculator-vga' if text else 'calculator-framebuffer')
    try:
        vm.wait('myos> ')
        if text:
            command(vm,'desktop','Desktop requires framebuffer >= 640x480')
            command(vm,'pwd','/\n')
            print('PASS: Calculator VGA text fallback leaves Shell usable',flush=True)
            return
        type_text(vm,'desktop\n');vm.wait('DESKTOP READY')
        vm.key('5');vm.wait('WINDOW OPEN 5');vm.wait('CALC: 0\n')
        addresses=symbols()
        sequence(vm,'2.5+1.25=','3.75')
        sequence(vm,'*2=','7.5')
        assert 'WINDOW OPEN 1' not in vm.log.read_text(), 'Calculator digits stole focus'
        press(vm,'c','0')
        sequence(vm,'1/3=','0.333')
        press(vm,'c','0')
        sequence(vm,'1-3=','-2')
        press(vm,'c','0')
        sequence(vm,'8/0=','Error: divide by zero')
        pixel=shot(vm,'calculator-error.ppm')
        assert pixel(448,368)==(0xd6,0x5c,0x69),pixel(448,368)
        press(vm,'c','0')
        sequence(vm,'1..','Error: invalid input')
        press(vm,'c','0')
        sequence(vm,'12\b+3=','4')
        press(vm,'c','0')
        sequence(vm,'999999+1=','Error: overflow')
        press(vm,'c','0')
        press(vm,'z','Error: invalid input')
        press(vm,'c','0')
        # Mouse uses the same input path as the keyboard. The fifth launcher
        # opens at (430,330); keypad centers are deterministic at 800x600.
        click(vm,addresses,474,418,'7')
        click(vm,addresses,698,514,'7') # '+' stores the left operand.
        click(vm,addresses,530,418,'8')
        click(vm,addresses,642,514,'15')
        move_to(vm,addresses,80,55)
        pixel=shot(vm,'calculator-framebuffer.ppm')
        assert pixel(448,368)==(0x97,0xa9,0xbd),pixel(448,368)
        ink={pixel(x,y) for y in range(374,390) for x in range(456,560)}
        assert (255,255,255) in ink, 'Calculator display has no rendered result'
        click(vm,addresses,698,418,'0') # Clear.
        click(vm,addresses,474,514,'0') # Zero.
        click(vm,addresses,474,418,'7')
        click(vm,addresses,698,450,'0') # Backspace.
        vm.key('q');vm.wait('DESKTOP CLOSED\nmyos> ')
        command(vm,'pwd','/\n')
        print('PASS: Calculator keyboard/mouse decimals, chaining, errors, clear/backspace, pixels and Shell',flush=True)
    finally:vm.close()

def low_memory():
    vm=VM('calculator-16M',ram=16)
    try:
        vm.wait('myos> ')
        type_text(vm,'desktop\n');vm.wait('DESKTOP READY')
        vm.key('5');vm.wait('WINDOW OPEN 5')
        sequence(vm,'2+2=','4')
        vm.key('q');vm.wait('DESKTOP CLOSED\nmyos> ')
        print('PASS: Calculator at 16 MiB RAM',flush=True)
    finally:vm.close()
