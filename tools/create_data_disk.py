"""Create a dedicated 2 MiB MyOS ATA data image. Existing files are preserved."""
from pathlib import Path
import sys
import argparse

SIGNATURE=b'MYOS-DATA-v1\0'
SIZE=2*1024*1024

def create_image(path,large=False):
    path=Path(path)
    path.parent.mkdir(parents=True,exist_ok=True)
    # Exclusive create: never replace an existing disk or user file.
    with path.open('xb') as file:
        file.write(b'MYOS-DATA-v2\0' if large else SIGNATURE)
        file.truncate(16*1024*1024 if large else SIZE)
    return path

if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('image',nargs='?',type=Path,default=Path('build/myos-data.img'))
    parser.add_argument('--large',action='store_true',help='create a new 16 MiB v2 disk with 64 KiB file slots')
    args=parser.parse_args()
    image=args.image
    try:create_image(image,large=args.large)
    except FileExistsError:raise SystemExit(f'File exists; keeping it unchanged: {image}')
    print(f'Created dedicated MyOS ATA data disk: {image} ({16*1024*1024 if args.large else SIZE} bytes)')
