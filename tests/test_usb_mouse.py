"""Phase 24: actual xHCI USB HID input, ring wrap, fallback and detach.
Uses 128 MiB focused firmware profile. No physical devices are accessed.
Run after normal BIOS/UEFI/USB image build: python3 tests/test_usb_mouse.py
"""
from pathlib import Path
import time
from test_system import VM,ROOT,OUT,symbols
from test_shell import command,type_text
from test_mouse import send_relative,send_button
from test_desktop import position
from test_firmware import desktop,FIRMWARE

def number(vm,addresses,name):
    return int.from_bytes(vm.memory(addresses[name],8),'little')

def wait_number(vm,addresses,name,minimum):
    until=time.monotonic()+5
    while time.monotonic()<until:
        value=number(vm,addresses,name)
        if value>=minimum:return value
        time.sleep(.01)
    raise AssertionError((name,minimum,value))

def exercise():
    image=ROOT/'build/myos-usb.img'
    import hashlib
    before=hashlib.sha256(image.read_bytes()).hexdigest()
    import subprocess,sys
    loop=OUT/'xhci-loop-data.img'
    loop.unlink(missing_ok=True) # Generated fixture only, never a user disk.
    subprocess.run([sys.executable,'tools/make_app_disk.py',str(loop),'build/loop.app','loop'],check=True)
    for name,firmware in [('bios',None),('uefi',FIRMWARE)]:
        vm=VM('xhci-usb-'+name,ram=128,boot_image=image,boot_bus='ide',usb=True,
              firmware=firmware,disk=loop if name=='uefi' else None,disk_index=0,
              xhci_devices=['usb-mouse,bus=xhci.0,id=usbmouse'])
        try:
            text=vm.wait('myos> ')
            assert 'USB: xHCI HID boot mouse initialized' in text,text
            addresses=symbols()
            assert position(vm,addresses)==(400,300)
            start=number(vm,addresses,'usb_mouse_reports')
            send_relative(vm,'x',40);send_relative(vm,'y',-20)
            wait_number(vm,addresses,'usb_mouse_reports',start+1)
            command(vm,'mouse','Mouse: x=440 y=280 buttons=0')
            send_button(vm,True);command(vm,'mouse','buttons=1')
            send_button(vm,False);command(vm,'mouse','buttons=0')
            desktop(vm)
            print(f'PASS: {name.upper()} exact USB boot on xHCI; native USB movement/buttons and three apps with PS/2 keyboard',flush=True)
            if name=='uefi':
                wrap_before=number(vm,addresses,'usb_ring_wraps')
                free_before=number(vm,addresses,'free_count')
                tick_before=number(vm,addresses,'ticks')
                # Wait for each USB report: QEMU cannot coalesce the whole
                # burst into one report and accidentally skip the cycle bit.
                for i in range(270):
                    count=number(vm,addresses,'usb_mouse_reports')
                    send_relative(vm,'x',1 if i%2 else -1)
                    wait_number(vm,addresses,'usb_mouse_reports',count+1)
                assert number(vm,addresses,'usb_ring_wraps')>wrap_before
                assert number(vm,addresses,'free_count')==free_before
                assert number(vm,addresses,'ticks')>tick_before+100
                # The high MMIO mapping must remain present under a Ring3
                # CR3, not merely while kernel desktop applications run.
                type_text(vm,'run /bin/loop\n')
                for i in range(40):
                    count=number(vm,addresses,'usb_mouse_reports')
                    send_relative(vm,'x',1 if i%2 else -1)
                    wait_number(vm,addresses,'usb_mouse_reports',count+1)
                assert 'error: app timed out' not in vm.log.read_text()
                vm.wait('error: app timed out')
                command(vm,'run /bin/hello','App exited with status 42')
                print('PASS: USB timer polling survives an uncooperative Ring3 address space and timeout/reaping',flush=True)
                # Disconnect while pressed must release shared mouse state.
                send_button(vm,True);command(vm,'mouse','buttons=1')
                disconnected=number(vm,addresses,'usb_disconnects')
                vm.command('device_del',{'id':'usbmouse'})
                wait_number(vm,addresses,'usb_disconnects',disconnected+1)
                command(vm,'mouse','buttons=0')
                command(vm,'pwd','/\n')
                print('PASS: USB producer/event cycle wrap, 270 actual reports, stable owned pages, timer and held-button detach recovery',flush=True)
        finally:vm.close()
    assert hashlib.sha256(image.read_bytes()).hexdigest()==before,'USB boot image changed'

    for name,devices,marker in [
        ('empty',[],None),
        ('unsupported',['usb-tablet,bus=xhci.0,id=tablet'],'USB: no supported boot mouse/BOT interface')]:
        vm=VM('xhci-'+name,ram=128,xhci_devices=devices)
        try:
            text=vm.wait('myos> ')
            assert 'USB: xHCI controller' in text,text
            assert 'USB: xHCI HID boot mouse initialized' not in text,text
            if marker:assert marker in text,text
            command(vm,'pwd','/\n')
            send_relative(vm,'x',20)
            # On unsupported tablet, QEMU routes pointer to the tablet; a
            # successful PS/2 keyboard/Shell remains the fallback assertion.
            print(f'PASS: xHCI {name} device retains responsive PS/2 keyboard/Shell without claiming HID support',flush=True)
        finally:vm.close()

if __name__=='__main__':exercise()
