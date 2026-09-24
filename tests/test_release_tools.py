"""Check release files through independent FAT tools and partition parsing."""
from pathlib import Path
import struct
import subprocess
import sys
import tempfile
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tools'))
from fat_image import make_esp
from package_usb import make_usb
def main():
    efi=(ROOT/'build/uefi/BOOTX64.EFI').read_bytes()
    esp=(ROOT/'build/myos-uefi.img').read_bytes()
    bios=(ROOT/'build/myos-bios.img').read_bytes()
    usb=(ROOT/'build/myos-usb.img').read_bytes()
    assert esp==make_esp(efi) and usb==make_usb(bios,esp)
    pe=struct.unpack_from('<I',efi,0x3c)[0]
    assert efi[pe:pe+4]==b'PE\0\0'
    assert struct.unpack_from('<I',efi,pe+8)[0]==0,'PE timestamp is not reproducible'
    active,_,kind,_,start,length=struct.unpack_from('<B3sB3sII',usb,446)
    assert (active,kind,start,length)==(0x80,0xef,2048,65536)
    assert usb[512:2048*512]==bios[512:2048*512]
    assert usb[:446]==bios[:446]
    with tempfile.TemporaryDirectory(dir=ROOT/'build',prefix='release-check-') as temp:
        path=Path(temp);volume=path/'esp.img';volume.write_bytes(usb[start*512:(start+length)*512])
        subprocess.run(['fsck.fat','-n',str(volume)],check=True)
        subprocess.run(['mcopy','-i',str(volume),'::/EFI/BOOT/BOOTX64.EFI',str(path/'readback.efi')],check=True)
        assert (path/'readback.efi').read_bytes()==efi
    for bad in (b'',b'broken'):
        try:make_usb(bad,esp)
        except ValueError:pass
        else:raise AssertionError('invalid BIOS input accepted')
    print('PASS: FAT filesystem checker, independent EFI readback, MBR layout and deterministic timestamps')
if __name__=='__main__':main()

