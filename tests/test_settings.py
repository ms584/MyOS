"""Phase 17 Settings widgets, validation, persistence and RAM-only QEMU tests."""
from pathlib import Path
import sys
import time
from test_system import VM,OUT,ROOT,symbols
from test_shell import command,type_text
from test_desktop import move_to,button,wait_count,shot
sys.path.insert(0,str(ROOT))
from tools.create_data_disk import create_image

DEFAULT='theme=dark background=blue accent=#276B8A'
CHANGED='theme=light background=green accent=#11AA33'

def click(vm,addresses,x,y,marker=None):
    move_to(vm,addresses,x,y)
    frames=vm.log.read_text().count('DESKTOP FRAME')
    count=vm.log.read_text().count(marker) if marker else 0
    button(vm,True)
    if marker:wait_count(vm,marker,count+1)
    button(vm,False)
    wait_count(vm,'DESKTOP FRAME',frames+1)

def field(vm,addresses,text):
    click(vm,addresses,430,458)
    for character in '\b'*6+text:
        frames=vm.log.read_text().count('DESKTOP FRAME')
        type_text(vm,character)
        wait_count(vm,'DESKTOP FRAME',frames+1)

def open_settings(vm,addresses,launcher=False):
    if launcher:click(vm,addresses,80,275,'WINDOW OPEN 4')
    else:
        frames=vm.log.read_text().count('DESKTOP FRAME')
        opened=vm.log.read_text().count('WINDOW OPEN 4')
        vm.key('4');wait_count(vm,'WINDOW OPEN 4',opened+1)
        wait_count(vm,'DESKTOP FRAME',frames+1)

def configure(vm,addresses,invalid=False):
    click(vm,addresses,560,352)
    click(vm,addresses,560,406)
    pixel=shot(vm,'settings-preview.ppm')
    assert pixel(200,300)==(0xe4,0xf2,0xe9),pixel(200,300)
    field(vm,addresses,'GGGGGG' if invalid else '11AA33')
    click(vm,addresses,620,458,
          'SETTINGS INVALID COLOR' if invalid else
          ('SETTINGS SAVED DISK' if 'ATA PIO disk' in vm.log.read_text() else 'SETTINGS SAVED RAM'))

def leave(vm):
    closed=vm.log.read_text().count('DESKTOP CLOSED')
    vm.key('q')
    wait_count(vm,'DESKTOP CLOSED',closed+1)
    deadline=time.monotonic()+5
    while not vm.log.read_text().endswith('myos> ') and time.monotonic()<deadline:
        time.sleep(.02)
    assert vm.log.read_text().endswith('myos> ')

def exercise(text=False):
    if text:
        vm=VM('settings-vga')
        try:
            vm.wait('myos> ')
            command(vm,'settings',DEFAULT+' persistence=RAM-only')
            command(vm,'desktop','Desktop requires framebuffer >= 640x480')
            print('PASS: Settings inspection and VGA text fallback',flush=True)
        finally:vm.close()
        return

    vm=VM('settings-ram')
    try:
        vm.wait('myos> ')
        command(vm,'settings',DEFAULT+' persistence=RAM-only')
        type_text(vm,'desktop\n');vm.wait('DESKTOP READY')
        addresses=symbols();open_settings(vm,addresses,launcher=True)
        configure(vm,addresses,invalid=True)
        leave(vm)
        command(vm,'settings',DEFAULT+' persistence=RAM-only')
        type_text(vm,'desktop\n')
        wait_count(vm,'DESKTOP READY',2)
        open_settings(vm,addresses)
        configure(vm,addresses)
        leave(vm)
        command(vm,'settings',CHANGED+' persistence=RAM-only')
    finally:vm.close()
    vm=VM('settings-ram-reboot')
    try:
        vm.wait('myos> ')
        command(vm,'settings',DEFAULT+' persistence=RAM-only')
    finally:vm.close()
    print('PASS: widget input validation, theme/background preview and RAM-only reset',flush=True)

    disk=OUT/'phase17-data.img'
    disk.unlink(missing_ok=True) # Only this ignored, disposable test fixture.
    create_image(disk)
    vm=VM('settings-disk-first',disk=disk)
    try:
        vm.wait('myos> ')
        command(vm,'settings',DEFAULT+' persistence=disk')
        type_text(vm,'desktop\n');vm.wait('DESKTOP READY')
        addresses=symbols();open_settings(vm,addresses,launcher=True)
        configure(vm,addresses)
        pixel=shot(vm,'settings-framebuffer.ppm')
        assert pixel(200,300)==(0xe4,0xf2,0xe9)
        leave(vm)
        command(vm,'settings',CHANGED+' persistence=disk')
    finally:vm.close()
    vm=VM('settings-disk-reboot',disk=disk)
    try:
        output=vm.wait('myos> ')
        assert 'SETTINGS RESTORED' in output,output
        command(vm,'settings',CHANGED+' persistence=disk')
        type_text(vm,'desktop\n');vm.wait('DESKTOP READY')
        pixel=shot(vm,'settings-restored.ppm')
        assert pixel(200,300)==(0xe4,0xf2,0xe9),pixel(200,300)
        leave(vm)
        command(vm,'write /etc/settings.cfg BAD')
    finally:vm.close()
    vm=VM('settings-invalid-file',disk=disk)
    try:
        output=vm.wait('myos> ')
        assert 'SETTINGS INVALID FILE' in output,output
        command(vm,'settings',DEFAULT+' persistence=disk')
    finally:vm.close()
    print('PASS: Settings disk save/reboot restore, strict file validation and defaults',flush=True)

def low_memory():
    vm=VM('settings-16M',ram=16)
    try:
        vm.wait('myos> ')
        type_text(vm,'desktop\n');vm.wait('DESKTOP READY')
        vm.key('4');vm.wait('WINDOW OPEN 4')
        vm.key('q');vm.wait('DESKTOP CLOSED\nmyos> ')
        print('PASS: Settings window at 16 MiB RAM',flush=True)
    finally:vm.close()
