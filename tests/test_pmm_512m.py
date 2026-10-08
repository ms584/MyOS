"""Real boot/exhaustion/upper-frame evidence; not Cobalt acceptance."""
from pathlib import Path
import sys
from test_system import ROOT,VM,symbols

uefi='--uefi' in sys.argv
for ram in ((64,256,512) if uefi else (16,256,512)):
    options=dict(ram=ram,cpu='max')
    if uefi:
        options.update(boot_image=ROOT/'build/myos-uefi.img',boot_bus='ide',
            firmware=(Path('/usr/share/OVMF/OVMF_CODE_4M.fd'),Path('/usr/share/OVMF/OVMF_VARS_4M.fd')))
    vm=VM(f'pmm512-{ram}'+('-uefi' if uefi else '-bios'),**options)
    try:
        serial=vm.wait('myos> ',timeout=120)
        assert 'PASS: PMM, paging, heap, exhaustion/recovery' in serial,serial
        count=int.from_bytes(vm.memory(symbols()['total_count'],8),'little')
        assert 0<count<ram*1024*1024//4096,(ram,count)
        high='PASS: PMM frames above256MiB, alias and retained lifetime' in serial
        assert high==(ram==512),(ram,count,serial)
        if ram==512:assert count>256*1024*1024//4096,count
        print('PASS actual managed pages/exhaustion/recovery/high-frame alias/refcount',ram,'UEFI' if uefi else 'BIOS',count,flush=True)
    finally:vm.close()
