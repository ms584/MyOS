"""Read-only inode observer safety/privacy tests; no media acceptance."""
import unittest,struct,json
from pathlib import Path
from unittest.mock import patch
from cobalt_inode_observer import InodeObserver
class VM:
    def __init__(self):self.calls=[];self.data=bytes(128*80);self.fail=False
    def command(self,name):self.calls.append(name);return {'status':'paused'}
    def memory(self,address,size):
        if self.fail:raise RuntimeError('read failure')
        assert size==128*80
        return self.data
class Tests(unittest.TestCase):
    def make(self,vm,symbols='00052a80 00002800 B nodes.lto_priv.0\n'):
        def read(p):return b'0'*(17*512)+b'kernel' if p.name=='myos-current.img' else b'kernel'
        with patch.object(Path,'read_bytes',read),patch('subprocess.check_output',return_value=symbols):return InodeObserver(vm)
    def test_capacity_and_name_privacy(self):
        vm=VM();data=bytearray(vm.data)
        struct.pack_into('<BBxxIIIQQ32sQQ',data,0,1,0,0,1,0,12,4096,b'PRIVATE-NAME',0x123456,0);vm.data=bytes(data)
        r=self.make(vm).capture();self.assertEqual(r['linked_file_buffer_capacity'],4096)
        self.assertNotIn('PRIVATE-NAME',json.dumps(r));self.assertNotIn('123456',json.dumps(r));self.assertEqual(vm.calls[-1],'cont')
    def test_finally_resume_on_read_error(self):
        vm=VM();vm.fail=True
        with self.assertRaises(RuntimeError):self.make(vm).capture()
        self.assertEqual(vm.calls[-1],'cont')
    def test_counter_bounds(self):
        vm=VM();data=bytearray(vm.data);struct.pack_into('<BBxxIIIQQ32sQQ',data,0,1,0,128,1,0,8192,4096,b'x',1,0);vm.data=bytes(data)
        with self.assertRaises(AssertionError):self.make(vm).capture()
        self.assertEqual(vm.calls[-1],'cont')
    def test_reject_layout_and_addresses(self):
        for s in ['00052a80 00002700 B nodes\n','00080000 00002800 B nodes\n','00052a80 00002800 B other\n','00052a80 00002800 B nodes\n00052a80 00002800 B nodes.lto_priv.1\n']:
            with self.assertRaises(AssertionError):self.make(VM(),s)
if __name__=='__main__':unittest.main()
