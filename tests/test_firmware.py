"""Native UEFI desktop/persistence and the exact dual-firmware USB image."""
import sys
from hashlib import sha256
from pathlib import Path
from test_system import VM,ROOT,OUT,symbols
from test_shell import command,type_text
from test_settings import click,configure,leave,CHANGED
from test_calculator import sequence
from test_desktop import shot,wait_count
sys.path.insert(0,str(ROOT))
from tools.create_data_disk import create_image

FIRMWARE=('/usr/share/OVMF/OVMF_CODE_4M.fd','/usr/share/OVMF/OVMF_VARS_4M.fd')
def desktop(vm):
    type_text(vm,'desktop\n');vm.wait('DESKTOP READY')
    addresses=symbols()
    click(vm,addresses,545,27,'WINDOW OPEN 4')
    click(vm,addresses,645,27,'WINDOW OPEN 5')
    sequence(vm,'7+8=','15')
    click(vm,addresses,742,27,'WINDOW OPEN 6')
    vm.wait('SNAKE SCORE 1',timeout=5)
    vm.key('p');vm.wait('SNAKE PAUSE')
    pixel=shot(vm,vm.name+'.ppm')
    assert pixel(200,300)==(0x11,0x1a,0x2c),pixel(200,300)
    leave(vm);command(vm,'pwd','/\n')

def main():
    image=ROOT/'build/myos-uefi.img'
    digest=sha256(image.read_bytes()).digest()
    disk=OUT/'phase22-data.img';disk.unlink(missing_ok=True);create_image(disk)
    vm=VM('uefi-desktop',ram=128,boot_image=image,boot_bus='ide',firmware=FIRMWARE,disk=disk,disk_index=1)
    try:
        text=vm.wait('myos> ',timeout=30)
        for marker in ['UEFI EXIT BOOT SERVICES OK','PASS: PMM, paging, heap, exhaustion/recovery',
                       'PASS: Ring 3 processes','PS/2 mouse initialized']:
            assert marker in text,text
        desktop(vm)
        ready=vm.log.read_text().count('DESKTOP READY')
        type_text(vm,'desktop\n');wait_count(vm,'DESKTOP READY',ready+1)
        addresses=symbols();click(vm,addresses,545,27,'WINDOW OPEN 4')
        configure(vm,addresses);leave(vm)
        command(vm,'settings',CHANGED+' persistence=disk')
        print('PASS: native UEFI memory/CPU handoff, three apps, PS/2 input and disk saves',flush=True)
    finally:vm.close()
    vm=VM('uefi-reboot',ram=128,boot_image=image,boot_bus='ide',firmware=FIRMWARE,disk=disk,disk_index=1)
    try:
        text=vm.wait('myos> ',timeout=30)
        assert 'SETTINGS RESTORED' in text and 'SNAKE HIGH RESTORED 1' in text,text
        command(vm,'settings',CHANGED+' persistence=disk')
        print('PASS: native UEFI reboot restores Settings and Snake score',flush=True)
    finally:vm.close()
    assert sha256(image.read_bytes()).digest()==digest,'UEFI image was modified'
    image=ROOT/'build/myos-usb.img';digest=sha256(image.read_bytes()).digest()
    for kind,firmware in [('bios',None),('uefi',FIRMWARE)]:
        vm=VM('usb-'+kind,ram=128,boot_image=image,boot_bus='ide',firmware=firmware,usb=True)
        try:
            text=vm.wait('myos> ',timeout=30)
            assert ('UEFI EXIT BOOT SERVICES OK' in text)==(kind=='uefi')
            desktop(vm)
            command(vm,'disk','unavailable (RAM only)')
            print(f'PASS: exact USB image boots {kind.upper()} via emulated USB storage; PS/2 desktop input',flush=True)
        finally:vm.close()
    assert sha256(image.read_bytes()).digest()==digest,'USB image was modified'

if __name__=='__main__':main()
