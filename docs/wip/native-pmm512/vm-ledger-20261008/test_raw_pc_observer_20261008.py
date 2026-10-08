"""Observer must always resume a paused VM, including parser failures."""
from pathlib import Path
import json,runpy,tempfile
Observer=runpy.run_path(str(Path(__file__).with_name('cobalt_raw_pc_observer_20261008.py')))['Observer']
class VM:
    def __init__(self,text,status='paused'):
        self.text=text;self.status=status;self.calls=[]
    def command(self,name,args=None):
        self.calls.append(name)
        if name=='query-status':return {'status':self.status}
        if name=='human-monitor-command':return self.text
        return {}
with tempfile.TemporaryDirectory() as directory:
    out=Path(directory)
    vm=VM('RIP=0000008006331234 CR3=0000000012345000\nCS =0033 0000000000000000')
    Observer(vm,out).capture(1,2)
    record=json.loads((out/'raw-current-pc.json').read_text())[0]
    assert record['cs']==0x33 and record['rip']==0x8006331234
    assert record['atomic_register_snapshot'] and not record['task_identity_valid']
    assert vm.calls==['stop','query-status','human-monitor-command','cont']
    for text,status in (('RIP=1234 CR3=1234','paused'),('','running')):
        vm=VM(text,status)
        try:Observer(vm,out).capture(1,2)
        except AssertionError:pass
        else:raise AssertionError('invalid snapshot accepted')
        assert vm.calls[-1]=='cont'
print('PASS raw register parsing, no metadata/task attribution, finally-resume on malformed CS/status')
