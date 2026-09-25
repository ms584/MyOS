"""Pack a bounded flat x86-64 MyOS app with a checked 16-byte header."""
import struct
import sys
from pathlib import Path
def checksum(data):
    value=2166136261
    for byte in data:value=((value^byte)*16777619)&0xffffffff
    return value
def main(source,target):
    code=Path(source).read_bytes()
    if not 0<len(code)<=65520:raise SystemExit('app code must fit the 64 KiB RAMFS file')
    Path(target).write_bytes(struct.pack('<8sII',b'MYOSAPP1',len(code),checksum(code))+code)
if __name__=='__main__':main(*sys.argv[1:])
