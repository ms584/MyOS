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
def slot(data,index,parent,kind,name,content=b''):
    offset=index*SLOT_BYTES
    struct.pack_into('<IIII',data,offset,1,parent,kind,len(content))
    data[offset+16:offset+16+len(name)]=name
    data[offset+512:offset+512+len(content)]=content
def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('output',type=Path)
    parser.add_argument('app',type=Path)
    parser.add_argument('name',help='filename under /bin, up to 31 ASCII letters/digits/_/-.')
    args=parser.parse_args()
    name=args.name
    if not (0<len(name)<32 and all(c.isascii() and (c.isalnum() or c in '_-.') for c in name)
            and name not in ('.','..')):raise SystemExit('invalid app filename')
    payload=args.app.read_bytes()
    if len(payload)<17 or len(payload)>4096 or payload[:8]!=b'MYOSAPP1':
        raise SystemExit('invalid MyOS app package')
    length,want=struct.unpack_from('<II',payload,8)
    if length!=len(payload)-16 or checksum(payload[16:])!=want:
        raise SystemExit('invalid MyOS app checksum')
    slots=bytearray(SLOT_BYTES*NODES)
    slot(slots,0,0,2,b'/')
    slot(slots,1,0,2,b'bin')
    slot(slots,2,1,1,name.encode(),payload)
    image=create_image(args.output)
    with image.open('r+b') as disk:
        disk.seek(BANK_START*512)
        header=bytearray(512)
        struct.pack_into('<QIIQI',header,0,MAGIC,1,577,1,checksum(slots))
        disk.write(header)
        disk.write(slots)
    print(f'Created {image} with /bin/{name}; source save disks untouched')
if __name__=='__main__':main()
