import os,subprocess,json,datetime,hashlib
from pathlib import Path
root=Path.cwd();identity=json.loads((root/'build/phase334-carrier-normal-identity.json').read_text())
base=os.environ.copy()
for k in list(base):
 if k.startswith('MYOS_MEDIA_') or k.startswith('MYOS_QEMU_'):base.pop(k)
provider=root/'build/media-evaluation/provider-sey3hj06'
results=[]
for name in ['faults','http403','timeout']:
 env=base.copy();env['MYOS_RENDERER_PROVIDER_PATH']=str(provider)
 if name=='faults':
  env['MYOS_MEDIA_BOOT_IMAGE']=str(root/'build/myos-current.img')
  cmd=['python3','tests/test_media_player_faults.py',str(provider),'--vp9']
 else:
  env.update(MYOS_MEDIA_FAULT=name,MYOS_MEDIA_NATIVE_PCM_CAPTURE='1',MYOS_MEDIA_REPEATS='1')
  cmd=['python3','tests/test_cobalt_media_observation.py','/home/xytron404/cd-QJYMwG/build/cobalt-resources-dwxalc2n','/home/xytron404/platform-fonts-01W7Ri/build/platform-fonts-20261007']
 log=root/('build/phase334-checkpoint11-current-'+name+'.log')
 with log.open('w') as f:r=subprocess.run(cmd,env=env,stdout=f,stderr=subprocess.STDOUT)
 results.append(dict(name=name,exit=r.returncode,command=cmd,log=str(log.relative_to(root)),date=datetime.datetime.now(datetime.timezone(datetime.timedelta(hours=7))).isoformat()))
 print(name,'exit',r.returncode,flush=True)
 assert r.returncode==0,log
after={n:hashlib.sha256((root/'build'/n).read_bytes()).hexdigest() for n in identity}
assert after==identity
(root/'build/phase334-checkpoint11-current-error-result.json').write_text(json.dumps(dict(phase_passed=False,focused_passed=True,steps=results,normal_identity_exact=True,normal_identity_after=after),indent=2)+'\n')
print('all latest focused native error/restart tests PASS; normal5 exact; not live expiry',flush=True)
