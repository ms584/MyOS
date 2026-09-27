"""Generate a deterministic MYV1 RLE/PCM sample and a NEW MyOS v2 disk."""
import argparse
import math
from pathlib import Path
import struct
from create_data_disk import create_image
from make_app_disk import BANK_START,MAGIC,NODES,checksum,slot

WIDTH,HEIGHT,FPS,FRAMES,RATE=80,60,10,30,11025
PALETTE=((0x10,0x20,0x38),(0x20,0x55,0x80),(0xf5,0xb6,0x42),(0x30,0xd0,0x8a))
SLOT_BYTES=66048

def encode_frame(index):
    pixels=[]
    left=2+2*index
    for y in range(HEIGHT):
        for x in range(WIDTH):
            color=2 if 20<=y<36 and left<=x<left+12 else 3 if y>=56 else 1 if 7<=y<12 else 0
            pixels.append(color)
    out=bytearray()
    value=pixels[0];count=0
    for color in pixels:
        if color!=value or count==255:
            out.extend((count,value));value=color;count=0
        count+=1
    out.extend((count,value))
    return bytes(out)

def make_clip():
    frames=[encode_frame(i) for i in range(FRAMES)]
    count=FRAMES*RATE//FPS
    pcm=bytes(128+int(70*math.sin(2*math.pi*(440 if i<count//2 else 660)*i/RATE))
              for i in range(count))
    header=struct.pack('<4sHHHHIIHH',b'MYV1',WIDTH,HEIGHT,FPS,FRAMES,RATE,count,len(PALETTE),0)
    palette=b''.join(bytes(rgb) for rgb in PALETTE)
    table=b''.join(struct.pack('<H',len(frame)) for frame in frames)
    clip=header+palette+table+b''.join(frames)+pcm
    if len(clip)>65536:raise ValueError('clip exceeds MyOS file limit')
    return clip

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('disk',type=Path)
    parser.add_argument('--clip',type=Path,default=Path('build/phase31-demo.myv'))
    parser.add_argument('--input',type=Path,help='pack an existing MYV1 clip instead of the built-in demo')
    parser.add_argument('--name',default='demo.myv',help='filename under /home')
    args=parser.parse_args()
    if args.disk.exists():raise SystemExit(f'refusing to overwrite {args.disk}')
    if not (0<len(args.name)<32 and all(c.isascii() and (c.isalnum() or c in '_-.') for c in args.name)
            and args.name not in ('.','..')):raise SystemExit('invalid MyOS filename')
    clip=args.input.read_bytes() if args.input else make_clip()
    if not clip or len(clip)>65536:raise SystemExit('MYV1 file must be 1..65536 bytes')
    if not args.input:
        args.clip.parent.mkdir(parents=True,exist_ok=True)
        args.clip.write_bytes(clip)
    slots=bytearray(SLOT_BYTES*NODES)
    slot(slots,0,0,2,b'/',slot_bytes=SLOT_BYTES)
    slot(slots,1,0,2,b'home',slot_bytes=SLOT_BYTES)
    slot(slots,2,1,1,args.name.encode(),clip,slot_bytes=SLOT_BYTES)
    for index,directory in enumerate((b'bin',b'etc',b'dev'),start=3):
        slot(slots,index,0,2,directory,slot_bytes=SLOT_BYTES)
    image=create_image(args.disk,large=True)
    with image.open('r+b') as disk:
        disk.seek(BANK_START*512)
        header=bytearray(512)
        struct.pack_into('<QIIQI',header,0,MAGIC,2,1+NODES*(SLOT_BYTES//512),1,checksum(slots))
        disk.write(header);disk.write(slots)
    print(f'Created {image} with /home/{args.name} ({len(clip)} bytes); source disks untouched')

if __name__=='__main__':main()
