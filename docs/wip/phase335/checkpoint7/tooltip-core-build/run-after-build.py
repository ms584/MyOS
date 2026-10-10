"""One-shot build-dependent private tooltip retest; never Phase acceptance."""
from pathlib import Path
import os,sys,time,json,re,subprocess,datetime
root=Path('/home/xytron404/myos');ws=root/'build/phase335-core-workspace';work=root/'build/phase335-abort-worktree'
log=root/'build/phase335-core-compile.log';result=ws/'after-build-result.json';pid=447750
proc=Path('/proc')/str(pid)
expected=str(ws/'src/third_party/ninja/ninja')+' -C out/evergreen-x64_gold -j12 cobalt'
assert proc.exists() and (proc/'cmdline').read_bytes().replace(b'\0',b' ').decode().strip()==expected
start=(proc/'stat').read_text().rsplit(') ',1)[1].split()[19]
state={'started_at':datetime.datetime.now().astimezone().isoformat(),'phase_passed':False,'drawing_verified':False,'owner_acceptance_received':False,'stage':'waiting-for-build','ninja_pid':pid,'test_scope':'private native HTTPS tooltip, not YouTube/audio acceptance'}
def save():
 state['updated_at']=datetime.datetime.now().astimezone().isoformat();result.write_text(json.dumps(state,indent=2)+'\n')
def run(args,cwd):
 print('STAGE',state['stage'],flush=True)
 subprocess.run([sys.executable,str(root/'tools/run_logged.py'),'--',sys.executable,'-u',*args],cwd=cwd,env=env,check=True)
env=os.environ.copy();env.pop('MYOS_SOURCE_CORE_MANIFEST',None)
save();print('Waiting only for actual owned Ninja447750 terminal state; no timers/tasks scheduled.',flush=True)
try:
 while proc.exists():
  try:
   if (proc/'stat').read_text().rsplit(') ',1)[1].split()[19]!=start:break
   if (proc/'cmdline').read_bytes().replace(b'\0',b' ').decode().strip()!=expected:break
  except FileNotFoundError:break
  time.sleep(2)
 deadline=time.monotonic()+30
 while time.monotonic()<deadline:
  with log.open('rb') as stream:
   stream.seek(max(0,log.stat().st_size-12000));tail=stream.read().decode(errors='replace')
  terminal=re.search(r'\] exit=(\d+)\s*$',tail)
  if terminal:break
  time.sleep(.25)
 assert terminal and terminal[1]=='0','No successful terminal compile log; no freeze/test allowed'
 state['stage']='freezing-built-core';save();run([str(ws/'freeze-core.py')],root)
 state['stage']='checking-other-vms';save()
 vms=[]
 for entry in Path('/proc').iterdir():
  if not entry.name.isdigit():continue
  try:
   if Path(os.readlink(entry/'exe')).name.startswith('qemu-system-'):vms.append(int(entry.name))
  except OSError:pass
 assert not vms,'Existing QEMU process(es) preserved; refusing parallel VM test: '+str(vms)
 manifest=ws/'core-build-identity.json';env['MYOS_TOOLTIP_CORE_MANIFEST']=str(manifest)
 resources=Path('/home/xytron404/cd-QJYMwG/build/cobalt-resources-dwxalc2n');fonts=Path('/home/xytron404/platform-fonts-01W7Ri/build/platform-fonts-20261007')
 assert resources.is_dir() and fonts.is_dir()
 state['stage']='running-private-tooltip-retest';save()
 run([str(work/'tests/phase335_tooltip_core_retest.py'),str(resources),str(fonts)],work)
 state['stage']='tooltip-survival-test-completed-visual-review-required';state['tooltip_test_exit']=0;save()
 print('Private tooltip survival test completed; PNG drawing review, normal online AV/regression and owner confirmation remain required.',flush=True)
except BaseException as error:
 state['stage']='stopped-before-acceptance';state['error']=str(error);save();raise
