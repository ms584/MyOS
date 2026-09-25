"""Exercise file-based Ring 3 C app loading through real QEMU keyboard input."""
import sys
from pathlib import Path
from test_system import VM,OUT,ROOT
from test_shell import command
sys.path.insert(0,str(ROOT/'tools'))
from create_data_disk import create_image
import subprocess

def exercise():
    data=OUT/'phase26-data.img'
    data.unlink(missing_ok=True)
    create_image(data)
    vm=VM('app-runtime',disk=data)
    try:
        vm.wait('myos> ')
        command(vm,'stat /bin/hello','file ')
        output=command(vm,'run /bin/hello','App exited with status 42')
        assert 'Hello from /bin/hello (C, Ring 3)' in output,output
        command(vm,'cp /bin/hello /home/hello')
        command(vm,'run /home/hello','App exited with status 42')
        command(vm,'write /home/bad invalid')
        command(vm,'run /home/bad','error: invalid executable')
        command(vm,'run /home/missing','error: not found')
        command(vm,'run /home','error: is a directory')
        command(vm,'cp /bin/hello /home/hello','error: already exists')
        command(vm,'mem','Free physical pages: ')
        print('PASS: C app loaded by path, Ring 3 syscalls/exit, binary copy and invalid-file rejection',flush=True)
    finally:vm.close()
    imported=OUT/'phase26-imported-data.img'
    imported.unlink(missing_ok=True)
    subprocess.run(['python3','tools/make_app_disk.py',str(imported),'build/port.app','port'],cwd=ROOT,check=True)
    vm=VM('app-runtime-imported',disk=imported)
    try:
        vm.wait('myos> ')
        output=command(vm,'run /bin/port','App exited with status 7')
        assert 'Ported C app: clock and console work' in output,output
        command(vm,'run /bin/hello','App exited with status 42')
        print('PASS: independent host-built C app imported on new data disk, tick/console ABI',flush=True)
    finally:vm.close()
    looping=OUT/'phase26-loop-data.img'
    looping.unlink(missing_ok=True)
    subprocess.run(['python3','tools/make_app_disk.py',str(looping),'build/loop.app','loop'],cwd=ROOT,check=True)
    vm=VM('app-runtime-loop',disk=looping)
    try:
        vm.wait('myos> ')
        command(vm,'run /bin/loop','error: app timed out')
        command(vm,'run /bin/hello','App exited with status 42')
        print('PASS: CPU-bound app times out, is reaped, and another app still runs',flush=True)
    finally:vm.close()
    large=OUT/'phase26-large-data.img'
    large.unlink(missing_ok=True)
    subprocess.run(['python3','tools/make_app_disk.py','--large',str(large),'build/capacity.app','capacity'],cwd=ROOT,check=True)
    vm=VM('app-runtime-large',disk=large)
    try:
        vm.wait('myos> ',timeout=40)
        output=command(vm,'run /bin/capacity','App exited with status 26')
        assert 'Large C app crossed a code page' in output,output
        command(vm,'cp /bin/capacity /home/capacity')
        command(vm,'stat /home/capacity','file ')
        print('PASS: 8 KiB+ C app crosses code page and copies to v2 disk',flush=True)
    finally:vm.close()
    vm=VM('app-runtime-large-reboot',disk=large)
    try:
        vm.wait('myos> ',timeout=40)
        command(vm,'run /home/capacity','App exited with status 26')
        print('PASS: multi-page executable persists across v2 reboot',flush=True)
    finally:vm.close()
    # The copied file is generation 2 in bank B. Corrupt that bank and make
    # sure the original generation 1 app remains bootable from bank A.
    newest=16+8257
    with large.open('r+b') as disk:
        disk.seek((newest+1+2*129+1)*512)
        byte=disk.read(1)
        assert byte
        disk.seek((newest+1+2*129+1)*512)
        disk.write(bytes([byte[0]^0xff]))
    vm=VM('app-runtime-large-recovery',disk=large)
    try:
        vm.wait('myos> ',timeout=40)
        assert 'restored RAMFS generation 1' in vm.log.read_text()
        command(vm,'run /bin/capacity','App exited with status 26')
        command(vm,'run /home/capacity','error: not found')
        print('PASS: v2 corrupt-bank recovery keeps previous executable',flush=True)
    finally:vm.close()
    browser=OUT/'phase26-browser-component.img'
    browser.unlink(missing_ok=True)
    subprocess.run(['python3','tools/make_app_disk.py','--large',str(browser),'build/wapcaplet.app','wapcaplet'],cwd=ROOT,check=True)
    vm=VM('app-runtime-browser-component',disk=browser)
    try:
        vm.wait('myos> ',timeout=40)
        output=command(vm,'run /bin/wapcaplet','App exited with status 26')
        assert 'NetSurf libwapcaplet Ring 3: string interning PASS' in output,output
        print('PASS: real NetSurf dependency ported to isolated Ring 3 with app heap/BSS',flush=True)
    finally:vm.close()
    vm=VM('app-runtime-reboot',disk=data)
    try:
        vm.wait('myos> ')
        command(vm,'run /home/hello','App exited with status 42')
        print('PASS: copied executable persists and runs after reboot',flush=True)
    finally:vm.close()

if __name__=='__main__':exercise()
