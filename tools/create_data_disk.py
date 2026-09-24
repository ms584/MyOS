"""Create a dedicated 2 MiB MyOS ATA data image. Existing files are preserved."""
from pathlib import Path
import sys

SIGNATURE=b'MYOS-DATA-v1\0'
SIZE=2*1024*1024

def create_image(path):
    path=Path(path)
    path.parent.mkdir(parents=True,exist_ok=True)
    # Exclusive create: never replace an existing disk or user file.
    with path.open('xb') as file:
        file.write(SIGNATURE)
        file.truncate(SIZE)
    return path

if __name__=='__main__':
    image=Path(sys.argv[1] if len(sys.argv)>1 else 'build/myos-data.img')
    try:create_image(image)
    except FileExistsError:raise SystemExit(f'File exists; keeping it unchanged: {image}')
    print(f'Created dedicated MyOS ATA data disk: {image} ({SIZE} bytes)')
