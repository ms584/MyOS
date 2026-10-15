"""Root-attached xHCI SCSI/BOT snapshots; only generated file fixtures.
BIOS/UEFI boot, files/settings/Snake reboot, damaged-bank fallback, ring wrap,
concurrent mouse input, rejects for foreign/overlap/marker/4K and disconnect.
"""
from pathlib import Path
import hashlib,struct,sys,shutil
from test_system import ROOT,OUT,VM,symbols
from test_shell import command,type_text
from test_firmware import FIRMWARE,desktop
from test_settings import configure,click,leave,CHANGED
from test_mouse import send_relative
from test_usb_mouse import number,wait_number
sys.path.insert(0,str(ROOT/'tools'))
from package_usb import make_usb,DATA_LBA,DATA_SECTORS
BANK_SECTORS=1+64*129
DATA_START=DATA_LBA*512
DATA_END=(DATA_LBA+DATA_SECTORS)*512
MAGIC=0x315346534f594d
def digest(path):return hashlib.sha256(path.read_bytes()).hexdigest()
def fixture(name,data):
    p=OUT/name
    p.write_bytes(data) # Named generated fixture inside isolated test output.
    return p
def bank(path,index):
    with path.open('rb') as f:
        f.seek((DATA_LBA+16+index*BANK_SECTORS)*512)
        raw=f.read(512)
    magic,version,sectors,generation=struct.unpack_from('<QIIQ',raw)
    assert (magic,version,sectors)==(MAGIC,2,BANK_SECTORS),(index,raw[:24])
    return generation
def unchanged_boot(path,initial):
    raw=path.read_bytes()
    assert raw[:DATA_START]==initial[:DATA_START],'MBR/BIOS/ESP/gap changed'
    assert raw[DATA_END:]==initial[DATA_END:],'writes escaped data partition'
    assert raw[DATA_START:DATA_START+512]==initial[DATA_START:DATA_START+512],'data marker changed'
def exercise():
    bios=(ROOT/'build/myos-bios.img').read_bytes()
    esp=(ROOT/'build/myos-uefi.img').read_bytes()
    initial=make_usb(bios,esp,True)
    original={str(p):digest(p) for p in [ROOT/'build/myos-bios.img',ROOT/'build/myos-uefi.img',ROOT/'build/myos-usb.img']}
    image=fixture('usb-bot-save.img',initial)
    mouse=['usb-mouse,bus=xhci.0,id=botmouse']
    vm=VM('usb-bot-first',ram=128,boot_image=image,boot_bus='ide',usb=True,firmware=FIRMWARE,xhci_devices=mouse)
    try:
        text=vm.wait('myos> ',timeout=120)
        assert 'USB BOT: prepared MyOS data partition, 32768 sectors (512B)' in text,text
        usb_info=vm.command('human-monitor-command',{'command-line':'info usb'})
        assert '5000' in usb_info,usb_info
        print('Verified SuperSpeed BOT fixture: '+usb_info.replace('\n','; '),flush=True)
        addresses=symbols();before=number(vm,addresses,'usb_ring_wraps')
        command(vm,'write /home/usb USB snapshot')
        command(vm,'disk','saved generation 1')
        command(vm,'cat /home/usb','USB snapshot')
        assert number(vm,addresses,'usb_ring_wraps')>before,'bulk producer ring did not wrap'
        count=number(vm,addresses,'usb_mouse_reports')
        send_relative(vm,'x',15);wait_number(vm,addresses,'usb_mouse_reports',count+1)
        desktop(vm) # Includes Snake high score1.
        type_text(vm,'desktop\n');vm.wait('DESKTOP READY')
        # desktop() exited, so use a counted open marker through helper.
        from test_desktop import wait_count
        ready_count=vm.log.read_text().count('DESKTOP READY')
        assert ready_count>=2
        click(vm,addresses,545,27,'WINDOW OPEN 4')
        configure(vm,addresses);leave(vm)
        command(vm,'settings',CHANGED+' persistence=disk')
    finally:vm.close()
    unchanged_boot(image,initial)
    newest=max(bank(image,0),bank(image,1))
    assert newest>=3
    for name,firmware in [('bios',None),('uefi',FIRMWARE)]:
        vm=VM('usb-bot-reboot-'+name,ram=128,boot_image=image,boot_bus='ide',usb=True,firmware=firmware,xhci_devices=[])
        try:
            text=vm.wait('myos> ',timeout=120)
            assert f'restored RAMFS generation {newest}' in text,text
            assert 'SETTINGS RESTORED' in text and 'SNAKE HIGH RESTORED 1' in text,text
            command(vm,'cat /home/usb','USB snapshot')
            command(vm,'settings',CHANGED+' persistence=disk')
        finally:vm.close()
    unchanged_boot(image,initial)
    # Damage only newest payload; older bank must remain usable.
    latest=0 if bank(image,0)==newest else 1
    with image.open('r+b') as f:
        at=(DATA_LBA+16+latest*BANK_SECTORS+1)*512
        f.seek(at);old=f.read(1);f.seek(at);f.write(bytes([old[0]^0xff]))
    vm=VM('usb-bot-recovery',ram=128,boot_image=image,boot_bus='ide',usb=True,firmware=FIRMWARE,xhci_devices=[])
    try:
        text=vm.wait('myos> ',timeout=120)
        assert f'restored RAMFS generation {newest-1}' in text,text
        command(vm,'cat /home/usb','USB snapshot')
    finally:vm.close()
    unchanged_boot(image,initial)
    print('PASS: xHCI/BOT512 UEFI writes, BIOS/UEFI reboot restore files/Settings/Snake; corrupted-bank fallback; bulk wrap and mouse coexistence; boot regions unchanged',flush=True)

    cases=[]
    foreign=bytearray(initial);foreign[462:478]=bytes(16);cases.append(('foreign',foreign,512))
    overlap=bytearray(initial);struct.pack_into('<I',overlap,470,2048);cases.append(('overlap',overlap,512))
    marker=bytearray(initial);marker[DATA_START]=0;cases.append(('bad-marker',marker,512))
    extra=bytearray(initial);extra[482]=0x83;cases.append(('extra-partition',extra,512))
    cases.append(('sector4096',initial,4096))
    for name,data,sector in cases:
        disk=fixture('usb-bot-'+name+'.img',data);before=digest(disk)
        vm=VM('usb-bot-'+name,ram=128,xhci_devices=[],usb_disk=disk,usb_sector_size=sector)
        try:
            text=vm.wait('myos> ',timeout=120)
            assert 'USB BOT: no supported prepared data partition; RAM-only' in text,text
            command(vm,'write /home/volatile RAM')
            command(vm,'disk','unavailable (RAM only)')
        finally:vm.close()
        assert digest(disk)==before,name+' disk modified'
    # Disconnect after successful mount must fail a save, not hang forever.
    disk=fixture('usb-bot-detach.img',initial);before=digest(disk)
    vm=VM('usb-bot-detach',ram=128,xhci_devices=mouse,usb_disk=disk)
    try:
        text=vm.wait('myos> ',timeout=120)
        assert 'USB BOT: prepared MyOS data partition' in text,text
        vm.command('device_del',{'id':'teststorage'})
        result=command(vm,'write /home/detached Fail closed')
        assert 'save failed' in result.lower() or 'storage' in result.lower(),result
        command(vm,'pwd','/\n')
    finally:vm.close()
    assert digest(disk)==before,'detached medium modified'
    assert original=={p:digest(Path(p)) for p in original},'packaged source images changed'
    print('PASS: foreign/overlapping/unmarked/extra-partition/4096B USB media remain unchanged; detach fails boundedly and Shell responds',flush=True)
def exercise_usb2():
    initial=make_usb((ROOT/'build/myos-bios.img').read_bytes(),(ROOT/'build/myos-uefi.img').read_bytes(),True)
    image=fixture('usb-bot-high-speed.img',initial)
    for boot in range(2):
        vm=VM('usb-bot-high-speed-'+str(boot),ram=128,xhci_devices=[],xhci_usb3=False,usb_disk=image)
        try:
            text=vm.wait('myos> ',timeout=120)
            assert 'USB BOT: prepared MyOS data partition' in text,text
            info=vm.command('human-monitor-command',{'command-line':'info usb'})
            assert '480' in info,info
            if not boot:command(vm,'write /home/usb2 High speed')
            else:command(vm,'cat /home/usb2','High speed')
        finally:vm.close()
    unchanged_boot(image,initial)
    print('PASS: USB2 high-speed BOT512 write/reboot restore through xHCI with SuperSpeed ports disabled; boot regions unchanged',flush=True)
if __name__=='__main__':
    exercise()
    exercise_usb2()
