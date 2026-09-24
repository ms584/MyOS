"""ATA PIO persistence: real QEMU IDE disk, reboot and corrupt-bank recovery."""
from pathlib import Path
import struct
from test_system import VM,OUT,ROOT
from test_shell import command
import sys
sys.path.insert(0,str(ROOT))
from tools.create_data_disk import create_image,SIGNATURE

BANK_SECTORS=1+64*9
BANK_START=(16,16+BANK_SECTORS)
MAGIC=0x315346534f594d

def generation(path,bank):
    with path.open('rb') as file:
        file.seek(BANK_START[bank]*512)
        sector=file.read(512)
    magic,version,sectors,gen=struct.unpack_from('<QIIQ',sector)
    assert magic==MAGIC and version==1 and sectors==BANK_SECTORS,(bank,sector[:24])
    return gen

def exercise():
    disk=OUT/'phase13-data.img'
    # This named test artifact is overwritten deliberately; no user disk is touched.
    disk.unlink(missing_ok=True)  # Only this named, ignored test fixture.
    create_image(disk)
    foreign=OUT/'phase13-foreign.img'
    foreign.unlink(missing_ok=True)
    foreign.write_bytes(bytes(2*1024*1024))
    vm=VM('storage-foreign',disk=foreign)
    try:
        vm.wait('myos> ')
        assert 'unrecognized data disk; RAM-only' in vm.log.read_text()
        command(vm,'mkdir /home/demo')
        command(vm,'disk','unavailable (RAM only)')
    finally:vm.close()
    assert foreign.read_bytes()==bytes(2*1024*1024),'Unknown disk was modified'
    vm=VM('storage-first',disk=disk)
    try:
        vm.wait('myos> ')
        content=vm.log.read_text()
        assert 'ATA PIO disk: 4096 sectors' in content,content
        assert 'empty data disk' in content,content
        command(vm,'mkdir /home/demo')
        command(vm,'write /home/demo/note Persistent MyOS')
        command(vm,'disk','saved generation 2')
        command(vm,'cat /home/demo/note','Persistent MyOS')
    finally:vm.close()
    assert generation(disk,0)==1 and generation(disk,1)==2
    assert disk.read_bytes()[:len(SIGNATURE)]==SIGNATURE,'Data disk signature was modified'

    vm=VM('storage-reboot',disk=disk)
    try:
        vm.wait('myos> ')
        assert 'restored RAMFS generation 2' in vm.log.read_text()
        command(vm,'cat /home/demo/note','Persistent MyOS')
        command(vm,'pwd','/\n')
        command(vm,'sync','Saved generation 3')
    finally:vm.close()
    assert generation(disk,0)==3

    # Damage the newest bank's file data. Its valid header must fail checksum,
    # and the previous committed bank must still provide the original file.
    with disk.open('r+b') as file:
        file.seek((BANK_START[0]+1+6*9+1)*512)
        old=file.read(1)
        file.seek((BANK_START[0]+1+6*9+1)*512)
        file.write(bytes([old[0]^0xff]))
    vm=VM('storage-recovery',disk=disk)
    try:
        vm.wait('myos> ')
        assert 'restored RAMFS generation 2' in vm.log.read_text()
        command(vm,'cat /home/demo/note','Persistent MyOS')
    finally:vm.close()
    # Now make that newest bank checksum-consistent but structurally invalid:
    # directory slot 6 points to an unused parent. Mount must still fall back.
    with disk.open('r+b') as file:
        file.seek((BANK_START[0]+1)*512)
        payload=bytearray(file.read(64*9*512))
        struct.pack_into('<I',payload,6*9*512+4,63)
        value=2166136261
        for byte in payload:value=((value^byte)*16777619)&0xffffffff
        file.seek((BANK_START[0]+1)*512);file.write(payload)
        file.seek(BANK_START[0]*512+24);file.write(struct.pack('<I',value))
    vm=VM('storage-invalid-tree',disk=disk)
    try:
        vm.wait('myos> ')
        assert 'restored RAMFS generation 2' in vm.log.read_text()
        command(vm,'cat /home/demo/note','Persistent MyOS')
    finally:vm.close()
    print('PASS: ATA PIO I/O, persistence, reboot, checksum and invalid-tree fallback',flush=True)
