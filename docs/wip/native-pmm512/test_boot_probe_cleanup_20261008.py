"""Normal-image boot probes must finish, while timers and real apps still work."""
from pathlib import Path
import time
from test_system import ROOT, VM, symbols
from test_shell import command

syms=symbols()
for uefi in (False,True):
    options=dict(ram=512)
    if uefi:
        options.update(boot_image=ROOT/'build/myos-uefi.img',boot_bus='ide',
            firmware=(Path('/usr/share/OVMF/OVMF_CODE_4M.fd'),
                      Path('/usr/share/OVMF/OVMF_VARS_4M.fd')))
    vm=VM('boot-probe-cleanup'+('-uefi' if uefi else '-bios'),**options)
    try:
        content=vm.wait('myos> ')
        for marker in ('PASS: two CPU-bound tasks preempted; task exit',
                       'PASS: context registers and direction flag preserved',
                       'PASS: Ring 3 processes, private CR3/data/stack, timer preemption',
                       'PASS: boot user probes retired and pages recovered',
                       'PASS: boot kernel probes retired'):
            assert marker in content,content
        before=vm.memory(syms['task_counts'],16)
        ticks=int.from_bytes(vm.memory(syms['ticks'],8),'little')
        assert all(int.from_bytes(before[i:i+8],'little')>0 for i in (0,8))
        time.sleep(1)
        assert vm.memory(syms['task_counts'],16)==before,'retired probe still executing'
        assert int.from_bytes(vm.memory(syms['ticks'],8),'little')>ticks+20,'timer stalled'
        command(vm,'echo boot-cleanup-shell-alive','boot-cleanup-shell-alive')
    finally:
        vm.close()
print('PASS BIOS/UEFI normal boot probes execute then retire; counters stable, timer/shell alive',flush=True)
