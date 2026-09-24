"""Build a 64 MiB MBR USB image bootable by BIOS and x86-64 UEFI firmware.

Writes only build/myos-usb.img. It never opens a physical block device.
"""
from pathlib import Path
import struct
ROOT=Path(__file__).resolve().parents[1]
SIZE=64*1024*1024
ESP_LBA=2048

def make_usb(bios,esp):
    if len(bios)<ESP_LBA*512 or bios[510:512]!=b'\x55\xaa':raise ValueError('Invalid BIOS image')
    if any(bios[446:510]):raise ValueError('BIOS code overlaps MBR partition table')
    if len(esp)!=32*1024*1024 or esp[510:512]!=b'\x55\xaa':raise ValueError('Invalid ESP')
    image=bytearray(SIZE)
    image[:ESP_LBA*512]=bios[:ESP_LBA*512]
    image[446:462]=struct.pack('<B3sB3sII',0x80,b'\xfe\xff\xff',0xef,b'\xfe\xff\xff',ESP_LBA,len(esp)//512)
    image[ESP_LBA*512:ESP_LBA*512+len(esp)]=esp
    struct.pack_into('<I',image,ESP_LBA*512+28,ESP_LBA)
    return image
def main():
    image=make_usb((ROOT/'build/myos-bios.img').read_bytes(),(ROOT/'build/myos-uefi.img').read_bytes())
    (ROOT/'build/myos-usb.img').write_bytes(image)
    print('Dual BIOS/UEFI USB image: build/myos-usb.img (64 MiB)',flush=True)
if __name__=='__main__':main()
