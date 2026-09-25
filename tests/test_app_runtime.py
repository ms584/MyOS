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
    vm=VM('app-runtime-reboot',disk=data)
    try:
        vm.wait('myos> ')
        command(vm,'run /home/hello','App exited with status 42')
        print('PASS: copied executable persists and runs after reboot',flush=True)
    finally:vm.close()

if __name__=='__main__':exercise()
