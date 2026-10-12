"""Prepare the tested normal native absolute-pointer bundle; never records acceptance."""
from pathlib import Path
import json,hashlib,datetime,subprocess,os
root=Path.cwd();build=root/'build';checkpoint=root/'docs/wip/phase335/checkpoint22'
verified=json.loads((checkpoint/'normal-build-identity.json').read_text())
assert verified.get('exit')==0 and verified.get('normal_restored_exact')
assert verified.get('normal_uefi_verified')
assert subprocess.run(['pgrep','-f','qemu-system-'],stdout=subprocess.DEVNULL).returncode!=0
identity=verified['final_normal_identity'];sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
assert all(sha(build/name)==value for name,value in identity.items())
old=build/'phase335-cursor-manual';previous=json.loads((old/'evidence.json').read_text())
frozen=json.loads((old/'manual-assets-identity.json').read_text())
assert all(sha(old/name)==value for name,value in frozen.items())
bundle=build/'phase335-absolute-manual';bundle.mkdir(exist_ok=False)
(bundle/'boot.img').write_bytes((build/'myos-current.img').read_bytes())
os.link(old/'assets.img',bundle/'assets.img')
(bundle/'platform-runtime.mrt').write_bytes((build/'platform-runtime.mrt').read_bytes())
evidence={'phase_passed':False,'observation_only':True,'prepared_only':True,'date':datetime.datetime.now().astimezone().isoformat(),'commit':subprocess.check_output(['git','rev-parse','HEAD'],text=True).strip(),'working_tree_dirty':bool(subprocess.check_output(['git','diff','--name-only'],text=True).strip()),'normal_identity':identity,'normal_identity_after':identity,'core_sha256':previous['core_sha256'],'source_core_build':previous['source_core_build'],'official_google_core':False,'provider_sha256':previous['provider_sha256'],'urls':previous['urls'],'mouse_absolute_driver':True,'provenance':'New normal native optional absolute pointer, unchanged immutable source core/provider. Requires actual owner input/online audiovisual acceptance.'}
(bundle/'evidence.json').write_text(json.dumps(evidence,indent=2)+'\n')
frozen={name:sha(bundle/name) for name in ['boot.img','assets.img','platform-runtime.mrt']}
(bundle/'manual-assets-identity.json').write_text(json.dumps(frozen,indent=2)+'\n')
(checkpoint/'manual-bundle-identity.json').write_text(json.dumps({'bundle':str(bundle),'frozen_inputs':frozen,'normal_identity':identity,'phase_passed':False},indent=2)+'\n')
print('Prepared normal absolute pointer bundle; asset hardlink avoids duplicate123MB; not accepted')
