"""Build a 64 MiB MBR USB image bootable by BIOS and x86-64 UEFI firmware.

Default output is build/myos-usb.img. --with-data creates a NEW writable
snapshot variant exclusively; it refuses to replace an existing save image.
It never opens a physical block device.
"""
from pathlib import Path
import struct
import argparse
ROOT=Path(__file__).resolve().parents[1]
SIZE=64*1024*1024
ESP_LBA=2048
DATA_LBA=69632
DATA_SECTORS=32768

def make_usb(bios,esp,with_data=False):
    if len(bios)<ESP_LBA*512 or bios[510:512]!=b'\x55\xaa':raise ValueError('Invalid BIOS image')
    if any(bios[446:510]):raise ValueError('BIOS code overlaps MBR partition table')
    if len(esp)!=32*1024*1024 or esp[510:512]!=b'\x55\xaa':raise ValueError('Invalid ESP')
    image=bytearray(SIZE)
    image[:ESP_LBA*512]=bios[:ESP_LBA*512]
    image[446:462]=struct.pack('<B3sB3sII',0x80,b'\xfe\xff\xff',0xef,b'\xfe\xff\xff',ESP_LBA,len(esp)//512)
    image[ESP_LBA*512:ESP_LBA*512+len(esp)]=esp
    struct.pack_into('<I',image,ESP_LBA*512+28,ESP_LBA)
    if with_data:
        image[462:478]=struct.pack('<B3sB3sII',0,b'\xfe\xff\xff',0xda,b'\xfe\xff\xff',DATA_LBA,DATA_SECTORS)
        image[DATA_LBA*512:DATA_LBA*512+13]=b'MYOS-DATA-v2\0'
    return image
def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--with-data',action='store_true',help='add a new 16 MiB MyOS v2 data partition')
    parser.add_argument('--output',type=Path,help='new generated .img file inside build/; never overwrite existing data')
    args=parser.parse_args()
    image=make_usb((ROOT/'build/myos-bios.img').read_bytes(),(ROOT/'build/myos-uefi.img').read_bytes(),args.with_data)
    target=args.output or ROOT/('build/myos-usb-data.img' if args.with_data else 'build/myos-usb.img')
    target=target.resolve()
    if not target.is_relative_to((ROOT/'build').resolve()) or target.suffix!='.img':
        raise SystemExit('Output must be a generated .img file inside build/')
    target.parent.mkdir(parents=True,exist_ok=True)
    try:
        with target.open('xb' if args.with_data or args.output else 'wb') as file:file.write(image)
    except FileExistsError:raise SystemExit(f'File exists; keeping it unchanged: {target}')
    print(f'Dual BIOS/UEFI USB image: {target} (64 MiB; data partition={args.with_data})',flush=True)
if __name__=='__main__':main()
