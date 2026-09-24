"""Small deterministic FAT16 ESP writer for EFI/BOOT/BOOTX64.EFI.

The fixed 32 MiB volume uses 512-byte sectors and four sectors per cluster.
All directory timestamps are 2000-01-01. No host filesystem is mounted.
"""
import struct
SIZE=32*1024*1024
SECTORS=SIZE//512
FAT_SECTORS=64
ROOT_START=1+2*FAT_SECTORS
DATA_START=ROOT_START+32

def make_esp(executable):
    image=bytearray(SIZE)
    image[:3]=b'\xeb\x3c\x90';image[3:11]=b'MYOS    '
    struct.pack_into('<HBHBHHBHHHII',image,11,512,4,1,2,512,0,0xf8,FAT_SECTORS,63,255,0,SECTORS)
    image[36:39]=b'\x80\0\x29'
    struct.pack_into('<I',image,39,0x4d594f53)
    image[43:54]=b'MYOS EFI   ';image[54:62]=b'FAT16   ';image[510:512]=b'\x55\xaa'
    fat=bytearray(FAT_SECTORS*512)
    for cluster,value in ((0,0xfff8),(1,0xffff),(2,0xffff),(3,0xffff)):
        struct.pack_into('<H',fat,cluster*2,value)
    count=(len(executable)+2047)//2048
    if not executable or count+4>(SECTORS-DATA_START)//4:raise ValueError('EFI executable too large')
    for i in range(count):struct.pack_into('<H',fat,(4+i)*2,0xffff if i+1==count else 5+i)
    image[512:(1+FAT_SECTORS)*512]=fat
    image[(1+FAT_SECTORS)*512:ROOT_START*512]=fat
    def entry(name,attr,cluster,size=0):
        data=bytearray(32);data[:11]=name;data[11]=attr
        struct.pack_into('<H',data,16,0x2821);struct.pack_into('<H',data,18,0x2821)
        struct.pack_into('<HHI',data,24,0x2821,cluster,size)
        return data
    def cluster_offset(n):return (DATA_START+(n-2)*4)*512
    image[ROOT_START*512:ROOT_START*512+32]=entry(b'EFI        ',0x10,2)
    image[ROOT_START*512+32:ROOT_START*512+64]=entry(b'MYOS EFI   ',0x08,0)
    for cluster,parent,name,next_cluster in ((2,0,b'BOOT       ',3),(3,2,b'BOOTX64 EFI',4)):
        at=cluster_offset(cluster)
        records=entry(b'.          ',0x10,cluster)+entry(b'..         ',0x10,parent)
        records+=entry(name,0x10 if cluster==2 else 0x20,next_cluster,0 if cluster==2 else len(executable))
        image[at:at+len(records)]=records
    at=cluster_offset(4);image[at:at+len(executable)]=executable
    return image
