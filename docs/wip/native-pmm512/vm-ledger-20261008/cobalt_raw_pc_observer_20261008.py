"""Paused actual CPU PC only; no saved frame/task metadata, no payload reads.

QMP/TCG sampling is biased and pauses execution. Counts identify investigation
candidates, never CPU percentages, thread identities or latency acceptance.
"""
import json,re,time
class Observer:
    def __init__(self,vm,out):
        self.vm=vm;self.out=out;self.records=[]
    def capture(self,launch,elapsed):
        began=time.monotonic();stopped=False
        try:
            self.vm.command('stop');stopped=True
            assert self.vm.command('query-status')['status']=='paused'
            text=self.vm.command('human-monitor-command',{'command-line':'info registers'})
            fields={}
            for name in ('RIP','CR3','CS'):
                match=re.search(r'\b'+name+r'\s*=\s*([0-9a-fA-F]+)',text)
                assert match,'missing register '+name
                fields[name.lower()]=int(match[1],16)
            self.records.append(dict(launch=launch,elapsed=elapsed,**fields,
                atomic_register_snapshot=True,task_identity_valid=False))
        finally:
            if stopped:self.vm.command('cont')
        self.records[-1]['pause_seconds']=time.monotonic()-began
        (self.out/'raw-current-pc.json').write_text(json.dumps(self.records,indent=2))
