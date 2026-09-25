"""Create a NEW 2 MiB MyOS data disk carrying one packaged app in /bin."""
import argparse
from pathlib import Path
import struct
from create_data_disk import create_image
from pack_app import checksum

SLOT_BYTES=4608
NODES=64
BANK_START=16
MAGIC=int.from_bytes(b'MYOSFS1','little')
def slot(data,index,parent,kind,name,content=b'',slot_bytes=SLOT_BYTES):
    offset=index*slot_bytes
    struct.pack_into('<IIII',data,offset,1,parent,kind,len(content))
    data[offset+16:offset+16+len(name)]=name
    data[offset+512:offset+512+len(content)]=content
def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('output',type=Path)
    parser.add_argument('app',type=Path)
    parser.add_argument('name',help='filename under /bin, up to 31 ASCII letters/digits/_/-.')
    parser.add_argument('--large',action='store_true',help='new v2 data disk with 64 KiB file slots')
    args=parser.parse_args()
    name=args.name
    if not (0<len(name)<32 and all(c.isascii() and (c.isalnum() or c in '_-.') for c in name)
            and name not in ('.','..')):raise SystemExit('invalid app filename')
    payload=args.app.read_bytes()
    if len(payload)<17 or len(payload)>(65536 if args.large else 4096) or payload[:8]!=b'MYOSAPP1':
        raise SystemExit('invalid MyOS app package')
    length,want=struct.unpack_from('<II',payload,8)
    if length!=len(payload)-16 or checksum(payload[16:])!=want:
        raise SystemExit('invalid MyOS app checksum')
    slot_bytes=66048 if args.large else SLOT_BYTES
    slots=bytearray(slot_bytes*NODES)
    slot(slots,0,0,2,b'/',slot_bytes=slot_bytes)
    slot(slots,1,0,2,b'bin',slot_bytes=slot_bytes)
    slot(slots,2,1,1,name.encode(),payload,slot_bytes=slot_bytes)
    for index,directory in enumerate((b"home",b"etc",b"dev"),start=3):
        slot(slots,index,0,2,directory,slot_bytes=slot_bytes)
    image=create_image(args.output,large=args.large)
    with image.open('r+b') as disk:
        disk.seek(BANK_START*512)
        header=bytearray(512)
        struct.pack_into('<QIIQI',header,0,MAGIC,2 if args.large else 1,1+NODES*(slot_bytes//512),1,checksum(slots))
        disk.write(header)
        disk.write(slots)
    print(f'Created {image} with /bin/{name}; source save disks untouched')
if __name__=='__main__':main()
