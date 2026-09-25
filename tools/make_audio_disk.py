"""Create a NEW MyOS v2 data disk containing one WAV file under /home.

Never overwrites an existing disk. The input must fit MyOS's 64 KiB file cap.
"""
import argparse
from pathlib import Path
import struct
from create_data_disk import create_image
from make_app_disk import slot,checksum,NODES,BANK_START,MAGIC

SLOT_BYTES=66048
def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('output',type=Path)
    parser.add_argument('wav',type=Path)
    parser.add_argument('--name',default='sample.wav')
    args=parser.parse_args()
    name=args.name
    if not (0<len(name)<32 and all(c.isascii() and (c.isalnum() or c in '_-.') for c in name)
            and name not in ('.','..')):
        raise SystemExit('invalid MyOS filename')
    payload=args.wav.read_bytes()
    if not payload or len(payload)>65536:raise SystemExit('WAV must be 1..65536 bytes')
    if args.output.exists():raise SystemExit(f'refusing to overwrite {args.output}')
    slots=bytearray(SLOT_BYTES*NODES)
    slot(slots,0,0,2,b'/',slot_bytes=SLOT_BYTES)
    slot(slots,1,0,2,b'home',slot_bytes=SLOT_BYTES)
    slot(slots,2,1,1,name.encode(),payload,slot_bytes=SLOT_BYTES)
    for index,directory in enumerate((b'bin',b'etc',b'dev'),start=3):
        slot(slots,index,0,2,directory,slot_bytes=SLOT_BYTES)
    image=create_image(args.output,large=True)
    with image.open('r+b') as disk:
        disk.seek(BANK_START*512)
        header=bytearray(512)
        struct.pack_into('<QIIQI',header,0,MAGIC,2,1+NODES*(SLOT_BYTES//512),1,checksum(slots))
        disk.write(header);disk.write(slots)
    print(f'Created {image} with /home/{name}; other disks untouched')
if __name__=='__main__':main()
