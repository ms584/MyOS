from pathlib import Path
import sys,json,runpy,subprocess
root=Path.cwd();cfg=Path((root/'build/phase334-netlog-config-path.txt').read_text().strip())
c=json.loads(cfg.read_text());out=cfg.parent
sys.path.insert(0,str(root/'tests'))
import test_system
from test_shell import command,type_text
def export_native_file(vm,path,timeout=600):
 import time
 offset=len(vm.log.read_text());type_text(vm,'cat '+path+chr(10));end=time.monotonic()+timeout
 while time.monotonic()<end:
  output=vm.log.read_text()[offset:]
  if output.endswith('myos> '):return output
  time.sleep(.25)
 raise RuntimeError('Native log export exceeded bounded600-second deadline; raw output remains private')

OriginalVM=test_system.VM
class ProbeVM(OriginalVM):
 def __init__(self,*args,**kwargs):
  kwargs['boot_image']=Path(c['boot_image']);super().__init__(*args,**kwargs);self.probe_ready=False
 def wait(self,needle,*args,**kwargs):
  result=super().wait(needle,*args,**kwargs)
  if needle=='myos> ' and not self.probe_ready:
   self.probe_ready=True
   for directory in ['/home','/home/cobalt','/home/cobalt/debug']:
    command(self,'mkdir '+directory,'myos> ')
  return result
 def close(self):
  try:
   before=self.log.read_text()
   if before.rstrip().endswith('myos>'):
    command(self,'ls /dev/shm','myos> ')
    export_native_file(self,c['log_path'])
    (out/'private-native-log-export.txt').write_text(self.log.read_text()[len(before):])
  finally:super().close()
test_system.VM=ProbeVM
sys.argv=['test_cobalt_youtube_observation.py','/home/xytron404/cd-QJYMwG/build/cobalt-resources-dwxalc2n','/home/xytron404/platform-fonts-01W7Ri/build/platform-fonts-20261007','--provider','build/media-evaluation/provider-sey3hj06','--identity','build/phase334-carrier-normal-identity.json','--memory-mib','1024','--seconds','150','--url','https://www.youtube.com/watch?v=aqz-KE-bpKQ','--engine-trace']
runpy.run_path(str(root/'tests/test_cobalt_youtube_observation.py'),run_name='__main__')
private=out/'private-native-log-export.txt'
t=private.read_text() if private.exists() else ''
result=dict(switch=c['switch'],export_exists=private.exists(),export_bytes=len(t),netlog_json_events_found='"events"' in t,phase_passed=False,normal_source_unchanged=True)
(out/'netlog-probe-result.json').write_text(json.dumps(result,indent=2)+'\n')
print('NETLOG_PROBE_SAFE_RESULT',json.dumps(result))
