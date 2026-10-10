"""Host safety/coverage tests for bounded native peak metadata; no media acceptance."""
import struct,unittest
from pathlib import Path
from unittest.mock import patch
from test_cobalt_pmm_observer import fake_file,SYMBOLS
from cobalt_pmm_history_observer import HistoryObserver
MORE='00070010 00000008 b ticks\n00071000 00002000 b pmm_pressure_history\n00073000 00000008 b heap_runtime_peak_bytes\n00073008 00000008 b heap_live_bytes\n'
class VM:
    def __init__(self):self.commands=[];self.tick=100;self.free=90;self.total=100;self.rows={};self.error=False
    def command(self,n):
        self.commands.append(n)
        if n=='query-status':return {'status':'paused'}
    def memory(self,address,size):
        if self.error and address==0x71000:raise RuntimeError('history read')
        if address==0x71000:
            assert size==8192
            values=[0]*1024
            for tag,free in self.rows.items():
                i=(tag-1)&511;values[2*i:2*i+2]=[tag,free]
            return struct.pack('<'+'QQ'*512,*values)
        assert size==8
        return {0x70000:self.free,0x70008:self.total,0x70010:self.tick,0x73000:1000,0x73008:900}[address].to_bytes(8,'little')
class Tests(unittest.TestCase):
    def make(self,vm,symbols=SYMBOLS+MORE):
        with patch.object(Path,'read_bytes',fake_file),patch('subprocess.check_output',return_value=symbols):return HistoryObserver(vm,1024)
    def test_transient_peak_and_boot_exclusion(self):
        vm=VM();o=self.make(vm);vm.rows={101:0};o.capture(1,0,'before-command')
        self.assertEqual(o.report()['history_owned_peak_bytes'],40960)
        vm.tick=110;vm.rows[106]=20;o.capture(1,1)
        r=o.report();self.assertEqual(r['sampled_peak_bytes'],40960);self.assertEqual(r['history_owned_peak_bytes'],80*4096);self.assertTrue(r['history_coverage_complete']);self.assertEqual(r['kernel_heap_runtime_peak_bytes'],1000)
    def test_gap_and_tick_regression(self):
        vm=VM();o=self.make(vm);o.capture(1,0);vm.tick+=513;o.capture(1,6);self.assertFalse(o.report()['history_coverage_complete'])
        vm.tick=100
        with self.assertRaises(AssertionError):o.capture(1,7)
        self.assertEqual(vm.commands[-1],'cont')
    def test_counter_and_read_errors_always_continue(self):
        for mode in ('counter','future','read'):
            vm=VM();o=self.make(vm);o.capture(1,0);vm.tick=105
            if mode=='counter':vm.rows={104:101}
            if mode=='future':vm.rows={107:50}
            if mode=='read':vm.error=True
            with self.assertRaises((AssertionError,RuntimeError)):o.capture(1,1)
            self.assertEqual(vm.commands[-1],'cont');self.assertEqual(len(o.history_samples),1)
    def test_reject_symbol_layout_and_bounds(self):
        for symbols in [SYMBOLS+MORE+MORE,SYMBOLS+MORE.replace('00002000','00001000'),SYMBOLS+MORE.replace('00071000','000ff000'),SYMBOLS]:
            with self.assertRaises(AssertionError):self.make(VM(),symbols)
if __name__=='__main__':unittest.main()
