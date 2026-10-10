"""Host-only observer safety tests; no guest execution or acceptance claims."""
from pathlib import Path
import unittest
from unittest.mock import patch
from cobalt_pmm_observer import Observer

def fake_file(path):
    return b'0'*(17*512)+b'kernel' if path.name=='myos-current.img' else b'kernel' if path.name=='kernel.bin' else b'elf'
SYMBOLS='00070000 00000008 b free_count\n00070008 00000008 b total_count\n'

class VM:
    def __init__(self):self.commands=[];self.status='paused';self.free=90;self.total=100;self.error=None
    def command(self,name):
        self.commands.append(name)
        if name=='query-status':return {'status':self.status}
    def memory(self,address,size):
        assert size==8
        if self.error:raise self.error
        return (self.free if address==0x70000 else self.total).to_bytes(8,'little')

class Tests(unittest.TestCase):
    def make(self,vm, symbols=SYMBOLS,memory_mib=512):
        with patch.object(Path,'read_bytes',fake_file),patch('subprocess.check_output',return_value=symbols):
            return Observer(vm,memory_mib=memory_mib)
    def test_valid_and_empty_report(self):
        vm=VM();observer=self.make(vm)
        self.assertIsNone(observer.report()['sampled_peak_bytes'])
        self.assertEqual(observer.capture(1,2)['pmm_allocated_bytes'],40960)
        self.assertEqual(vm.commands,['stop','query-status','cont'])
        self.assertEqual(observer.report()['sampled_peak_bytes'],40960)
    def test_finally_continue_on_errors(self):
        for mode in ('bad-counter','not-paused','read-error'):
            with self.subTest(mode=mode):
                vm=VM();observer=self.make(vm)
                if mode=='bad-counter':vm.free=101
                if mode=='not-paused':vm.status='running'
                if mode=='read-error':vm.error=RuntimeError('read failed')
                with self.assertRaises((AssertionError,RuntimeError)):observer.capture(1,0)
                self.assertEqual(vm.commands[-1],'cont')
                self.assertEqual(observer.samples,[])
    def test_explicit_memory_bound(self):
        vm=VM();vm.total=257751
        self.assertEqual(self.make(vm,memory_mib=1024).capture(1,0)['total_pages'],257751)
        with self.assertRaises(AssertionError):self.make(vm).capture(1,0)
        self.assertEqual(vm.commands[-1],'cont')
        vm.total=262145
        with self.assertRaises(AssertionError):self.make(vm,memory_mib=1024).capture(1,0)
        self.assertEqual(vm.commands[-1],'cont')
        with self.assertRaises(AssertionError):self.make(vm,memory_mib=2048)

    def test_reject_bad_symbols(self):
        for symbols in (SYMBOLS+SYMBOLS,SYMBOLS.replace('00070000','01000000'),
                        SYMBOLS.replace('00000008','00000004'),SYMBOLS.splitlines()[0]):
            with self.assertRaises(AssertionError):self.make(VM(),symbols)

if __name__=='__main__':unittest.main()
