"""Read-only sampled PMM usage from this exact kernel; not an exact peak."""
import hashlib
import subprocess
import time
from pathlib import Path

class Observer:
    def __init__(self, vm, memory_mib=512):
        assert memory_mib in (512,1024)
        self.memory_mib = memory_mib
        self.vm = vm
        self.samples = []
        self.symbols = {}
        self.paused_seconds = 0
        root = Path.cwd()
        self.kernel_elf_sha256 = hashlib.sha256((root/'build/kernel.elf').read_bytes()).hexdigest()
        kernel = (root/'build/kernel.bin').read_bytes()
        assert (root/'build/myos-current.img').read_bytes()[17*512:17*512+len(kernel)] == kernel
        for line in subprocess.check_output(['nm','-S','build/kernel.elf'],text=True).splitlines():
            fields = line.split()
            if len(fields) != 4: continue
            name = fields[3].split('.lto_priv.')[0]
            if name in ('free_count','total_count'):
                assert name not in self.symbols, 'ambiguous PMM symbol'
                address,size = int(fields[0],16),int(fields[1],16)
                assert size == 8 and address < 0x100000, 'PMM counter not identity mapped'
                self.symbols[name] = address
        assert set(self.symbols) == {'free_count','total_count'}

    def capture(self, launch, elapsed, label='sample'):
        began = time.monotonic()
        self.vm.command('stop')
        try:
            assert self.vm.command('query-status')['status'] == 'paused'
            values = {key:int.from_bytes(self.vm.memory(address,8),'little')
                      for key,address in self.symbols.items()}
            free,total = values['free_count'],values['total_count']
            assert 0 <= free <= total <= self.memory_mib*1024*1024//4096
            item = dict(launch=launch,elapsed=elapsed,label=label,free_pages=free,
                        total_pages=total,pmm_allocated_bytes=(total-free)*4096)
            self.samples.append(item)
            return item
        finally:
            self.vm.command('cont')
            self.paused_seconds += time.monotonic()-began

    def report(self):
        return dict(kernel_elf_sha256=self.kernel_elf_sha256,samples=self.samples,
                    sampled_peak_bytes=max((x['pmm_allocated_bytes'] for x in self.samples),default=None),
                    paused_seconds=self.paused_seconds,
                    caveat='Sampled PMM allocated frames, includes kernel/other residents; not exact peak or per-process RSS; heap backing reserved outside PMM is not counted.')
