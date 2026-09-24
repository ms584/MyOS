"""Desktop 1.0 acceptance: all three apps, errors, reboot and Shell."""
import re
import sys
from test_system import VM,OUT,ROOT,symbols
from test_shell import command,type_text
from test_desktop import shot
from test_settings import click,configure,leave,CHANGED
from test_calculator import press,sequence
sys.path.insert(0,str(ROOT))
from tools.create_data_disk import create_image

def exercise(text=False):
    if text:
        vm=VM('release-vga')
        try:
            vm.wait('myos> ')
            command(vm,'desktop','Desktop requires framebuffer >= 640x480')
            command(vm,'pwd','/\n')
            print('PASS: Desktop 1.0 VGA diagnostic Shell fallback',flush=True)
        finally:vm.close()
        return

    disk=OUT/'phase20-integration-data.img'
    disk.unlink(missing_ok=True) # Only the disposable fixture is replaced.
    create_image(disk)
    vm=VM('release-first',disk=disk)
    try:
        vm.wait('myos> ')
        type_text(vm,'desktop\n');vm.wait('DESKTOP READY')
        addresses=symbols()
        click(vm,addresses,545,27,'WINDOW OPEN 4') # Primary Settings toolbar.
        configure(vm,addresses,invalid=True)
        assert 'SETTINGS INVALID COLOR' in vm.log.read_text()
        configure(vm,addresses)
        click(vm,addresses,645,27,'WINDOW OPEN 5') # Calculator toolbar.
        sequence(vm,'8/0=','Error: divide by zero')
        pixel=shot(vm,'release-calculator-error.ppm')
        assert pixel(448,368)==(0xd6,0x5c,0x69),pixel(448,368)
        press(vm,'c','0')
        sequence(vm,'7+8=','15')
        click(vm,addresses,742,27,'WINDOW OPEN 6') # Snake toolbar.
        vm.wait('SNAKE SCORE 1',timeout=4)
        vm.wait('SNAKE HIGH SAVED 1',timeout=6)
        vm.key('p');vm.wait('SNAKE PAUSE')
        pixel=shot(vm,'desktop-1.0.ppm')
        assert pixel(200,300)==(0xe4,0xf2,0xe9),pixel(200,300)
        board={pixel(x,y) for y in range(414,526) for x in range(500,780)}
        assert (0x92,0xe4,0x6b) in board,'Snake disappeared in integrated desktop'
        vm.key('p');vm.wait('SNAKE RESUME')
        click(vm,addresses,545,27,'WINDOW FOCUS 4')
        vm.wait('SNAKE GAME OVER',timeout=10)
        click(vm,addresses,742,27,'WINDOW FOCUS 6')
        vm.key('r');vm.wait('SNAKE RESTART 0')
        vm.key('p');vm.wait('SNAKE PAUSE 0')
        highs=re.findall(r'SNAKE HIGH SAVED (\d+)\n',vm.log.read_text())
        assert highs,vm.log.read_text()
        saved_high=int(highs[-1])
        leave(vm)
        command(vm,'settings',CHANGED+' persistence=disk')
        command(vm,'stat /etc/settings.cfg','file 16 bytes')
        command(vm,'stat /home/snake.score','file 12 bytes')
        command(vm,'pwd','/\n')
        command(vm,'cat /etc/motd','Welcome to MyOS RAMFS.')
        print('PASS: all three apps in one desktop, error recovery, saved data and usable Shell',flush=True)
    finally:vm.close()

    vm=VM('release-reboot',disk=disk)
    try:
        output=vm.wait('myos> ')
        assert 'SETTINGS RESTORED' in output,output
        assert f'SNAKE HIGH RESTORED {saved_high}' in output,output
        command(vm,'settings',CHANGED+' persistence=disk')
        type_text(vm,'desktop\n');vm.wait('DESKTOP READY')
        pixel=shot(vm,'desktop-1.0-restored.ppm')
        assert pixel(200,300)==(0xe4,0xf2,0xe9),pixel(200,300)
        addresses=symbols()
        click(vm,addresses,742,27,'WINDOW OPEN 6')
        vm.wait(f'SNAKE START {saved_high}')
        vm.key('p');vm.wait('SNAKE PAUSE')
        leave(vm)
        command(vm,'pwd','/\n')
        print('PASS: both Settings and Snake high score restored after one reboot',flush=True)
    finally:vm.close()

def low_memory():
    vm=VM('release-16M',ram=16)
    try:
        vm.wait('myos> ')
        type_text(vm,'desktop\n');vm.wait('DESKTOP READY')
        addresses=symbols()
        for x,marker in ((545,'WINDOW OPEN 4'),(645,'WINDOW OPEN 5'),
                         (742,'WINDOW OPEN 6')):
            click(vm,addresses,x,27,marker)
        vm.key('p');vm.wait('SNAKE PAUSE')
        leave(vm)
        command(vm,'pwd','/\n')
        print('PASS: three Desktop 1.0 apps coexist at 16 MiB RAM',flush=True)
    finally:vm.close()
