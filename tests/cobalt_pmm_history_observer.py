"""Read existing bounded native PMM history, never reset counters or read guest heaps.
Peak covers runtime after the initial partial tick; coverage gaps are explicit.
Kernel heap backing pages are already PMM-owned; do not add heap-live bytes to RAM.
"""
import struct,subprocess,time
from cobalt_pmm_observer import Observer
class HistoryObserver(Observer):
    def __init__(self,vm,memory_mib=512):
        super().__init__(vm,memory_mib)
        self.history_symbols={};self.history_samples=[];self.baseline_tag=None;self.last_tag=None
        for line in subprocess.check_output(['nm','-S','build/kernel.elf'],text=True).splitlines():
            f=line.split()
            if len(f)!=4:continue
            name=f[3].split('.lto_priv.')[0]
            if name not in ('ticks','pmm_pressure_history','heap_runtime_peak_bytes','heap_live_bytes'):continue
            assert name not in self.history_symbols,'ambiguous history symbol'
            address,size=int(f[0],16),int(f[1],16)
            assert size==(8192 if name=='pmm_pressure_history' else 8) and address+size<0x100000
            self.history_symbols[name]=address
        assert len(self.history_symbols)==4
    def capture(self,launch,elapsed,label='sample'):
        began=time.monotonic();self.vm.command('stop')
        try:
            assert self.vm.command('query-status')['status']=='paused'
            read=lambda key:int.from_bytes(self.vm.memory((self.symbols|self.history_symbols)[key],8),'little')
            free,total,tick=read('free_count'),read('total_count'),read('ticks')
            assert 0<=free<=total<=self.memory_mib*1024*1024//4096
            live,heap_peak=read('heap_live_bytes'),read('heap_runtime_peak_bytes')
            assert 0<=live<=heap_peak<=self.memory_mib*1024*1024
            tag=tick+1
            if self.baseline_tag is None:self.baseline_tag=tag
            assert self.last_tag is None or tag>=self.last_tag
            values=struct.unpack('<'+'QQ'*512,self.vm.memory(self.history_symbols['pmm_pressure_history'],8192))
            relevant=[]
            for i in range(512):
                stamp,count=values[2*i:2*i+2]
                if not stamp:continue
                assert (stamp-1)&511==i and stamp<=tag
                if self.baseline_tag<stamp and (self.last_tag is None or stamp>=self.last_tag):
                    assert 0<=count<=total;relevant.append(count)
            gap=self.last_tag is not None and tag-self.last_tag>512
            minimum=min([free]+relevant)
            item=dict(launch=launch,elapsed=elapsed,label=label,free_pages=free,total_pages=total,pmm_allocated_bytes=(total-free)*4096)
            self.samples.append(item)
            self.history_samples.append(dict(launch=launch,elapsed=elapsed,tick=tick,start_tag=self.baseline_tag,previous_tag=self.last_tag,coverage_gap=gap,min_free_pages=minimum,pmm_owned_peak_bytes=(total-minimum)*4096,heap_live_bytes=live,heap_runtime_peak_bytes=heap_peak))
            self.last_tag=tag;return item
        finally:
            self.vm.command('cont');self.paused_seconds+=time.monotonic()-began
    def report(self):
        r=super().report();rows=self.history_samples
        r.update(history_samples=rows,history_coverage_complete=bool(rows) and not any(x['coverage_gap'] for x in rows),history_owned_peak_bytes=max((x['pmm_owned_peak_bytes'] for x in rows),default=None),kernel_heap_runtime_peak_bytes=max((x['heap_runtime_peak_bytes'] for x in rows),default=None),history_scope='PMM allocation low-water in existing512x10ms ring after initial partial baseline tick, excluding boot OOM. Poll gaps>512ticks invalidate complete peak coverage. PMM includes heap backing pages; cumulative heap high-water is reported separately, never added to physical RAM. Includes kernel/other residents, not per-process RSS or whole boot peak.')
        return r
