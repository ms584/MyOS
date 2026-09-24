"""Boot the exact packaged BIOS HDD image in QEMU, with and without data disk."""
from hashlib import sha256
from pathlib import Path
import sys

from test_system import VM,ROOT,OUT,symbols
from test_shell import command,type_text
from test_settings import click,configure,leave,CHANGED
from test_desktop import shot
sys.path.insert(0,str(ROOT))
from tools.create_data_disk import create_image

IMAGE=ROOT/'build/myos-bios.img'

def boot(name,disk=None):
    return VM(name,boot_image=IMAGE,boot_bus='ide',disk=disk,disk_index=1)

def main():
    # The packaged image, not the development floppy, is the test subject.
    import subprocess
    subprocess.run([sys.executable,str(ROOT/'tools/package_bios.py')],cwd=ROOT,check=True)
    data=IMAGE.read_bytes()
    assert len(data)==16*1024*1024 and data[510:512]==b'\x55\xaa'
    kernel=(ROOT/'build/kernel.bin').read_bytes()
    assert data[17*512:17*512+len(kernel)]==kernel
    assert (len(kernel)+511)//512>46,'HDD kernel read did not cross the floppy CHS boundary'
    digest=sha256(data).digest()
    vm=boot('bios-hdd-standalone')
    try:
        output=vm.wait('myos> ')
        assert '64-bit Long Mode' in output and 'framebuffer 800x600x32' in output
        command(vm,'disk','unavailable (RAM only)')
        type_text(vm,'desktop\n');vm.wait('DESKTOP READY')
        addresses=symbols()
        for x,marker in ((545,'WINDOW OPEN 4'),(645,'WINDOW OPEN 5'),(742,'WINDOW OPEN 6')):
            click(vm,addresses,x,27,marker)
        pixel=shot(vm,'bios-hdd-desktop.ppm')
        assert pixel(200,300)==(0x11,0x1a,0x2c),pixel(200,300)
        leave(vm)
        command(vm,'pwd','/\n')
        print('PASS: distributed BIOS HDD image boots Desktop 1.0 without a data disk',flush=True)
    finally:vm.close()
    assert sha256(IMAGE.read_bytes()).digest()==digest,'OS image changed during RAM-only boot'

    disk=OUT/'phase21-data.img'
    disk.unlink(missing_ok=True) # Disposable ignored test fixture only.
    create_image(disk)
    vm=boot('bios-hdd-save',disk)
    try:
        output=vm.wait('myos> ')
        assert 'ATA PIO disk: 4096 sectors' in output,output
        type_text(vm,'desktop\n');vm.wait('DESKTOP READY')
        addresses=symbols()
        click(vm,addresses,545,27,'WINDOW OPEN 4')
        configure(vm,addresses)
        click(vm,addresses,742,27,'WINDOW OPEN 6')
        vm.wait('SNAKE HIGH SAVED 1',timeout=5)
        leave(vm)
        command(vm,'settings',CHANGED+' persistence=disk')
        command(vm,'disk','saved generation')
        print('PASS: primary-slave data disk receives Settings and Snake saves',flush=True)
    finally:vm.close()
    vm=boot('bios-hdd-reboot',disk)
    try:
        output=vm.wait('myos> ')
        assert 'SETTINGS RESTORED' in output and 'SNAKE HIGH RESTORED 1' in output,output
        command(vm,'settings',CHANGED+' persistence=disk')
        command(vm,'pwd','/\n')
        print('PASS: exact BIOS image restores both app settings after reboot',flush=True)
    finally:vm.close()
    assert sha256(IMAGE.read_bytes()).digest()==digest,'OS image changed during persistence test'

if __name__=='__main__':main()
