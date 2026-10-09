from pathlib import Path
import subprocess,json,datetime,hashlib
steps=[('full',['python3','tests/test_system.py']),('normal-uefi',['python3','tools/build_uefi.py']),('launcher-uefi',['python3','tests/test_starboard_launcher.py','--uefi']),('pair-bios',['python3','tests/test_elf_pair_starboard.py','--provider','build/renderer-provider-evaluation/provider-84sokh6g']),('pair-uefi',['python3','tests/test_elf_pair_starboard.py','--provider','build/renderer-provider-evaluation/provider-84sokh6g','--uefi'])]
result={'date_start':datetime.datetime.now().astimezone().isoformat(),'steps':[],'phase_passed':False,'full_passed':False,'normal_identity_verified':False}
p=Path('build/phase334-checkpoint6-regression-result.json')
for name,cmd in steps:
 log=Path('build/phase334-checkpoint6-'+name+'.log')
 print('CHECKPOINT6_REGRESSION_START',name,str(log),flush=True)
 with log.open('w') as out:
  code=subprocess.run(['python3','tools/run_logged.py','--',*cmd],stdout=out,stderr=subprocess.STDOUT).returncode
 result['steps'].append({'name':name,'command':cmd,'log':str(log),'exit':code})
 p.write_text(json.dumps(result,indent=2)+chr(10))
 print('CHECKPOINT6_REGRESSION_RESULT',name,code,flush=True)
 if code:raise SystemExit(code)
 if name=='full':result['full_passed']=True
identity=json.loads(Path('build/phase334-carrier-normal-identity.json').read_text())
actual={f:hashlib.sha256(Path('build',f).read_bytes()).hexdigest() for f in identity}
result['normal_identity_after']=actual
result['normal_identity_verified']=actual==identity
result['date_end']=datetime.datetime.now().astimezone().isoformat()
p.write_text(json.dumps(result,indent=2)+chr(10))
assert result['normal_identity_verified'],'Normal identity mismatch, inspect and document actual build'
print('CHECKPOINT6_FULL_AND_FINAL_CHECKS_PASS_NORMAL_IDENTITY_EXACT',flush=True)
