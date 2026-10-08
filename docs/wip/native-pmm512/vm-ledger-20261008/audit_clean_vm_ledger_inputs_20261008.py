"""Read-only main/candidate input audit; never asserts unfinished tests passed."""
from pathlib import Path
import hashlib,json,subprocess
main=Path('/home/xytron404/myos');trial=Path('/home/xytron404/pmm-syscall-R4sQZZ')
stage=Path(__file__).resolve().parent
names=subprocess.check_output(['git','ls-files','-z','--','build.sh','boot','cpu','memory','include','kernel','drivers','fs','net','gui','video','audio','user','shell','security','tests','tools'],cwd=main).decode().split('\0')
expected={'memory/process.c':stage/'process-vm-ledger-clean-20261008.c',
          'tests/test_desktop.py':stage/'test-desktop-frame-fence-20261008.py'}
external={'tests/test_named_shm_lifetime.py':stage/'test_named_shm_lifetime_20261008.py',
          'user/elf-named-shm-lifetime.c':stage/'elf-named-shm-lifetime-20261008.c'}
def normalized_named_driver(path):
    text=path.read_text()
    if path==external['tests/test_named_shm_lifetime.py']:
        changes=[('"""Named SHM persists, process aliases do not; no global page-check bypass."""',
                  '"""Named SHM is filesystem residency; process aliases must still be reclaimed."""'),
                 ('root=Path.cwd();stage=Path(__file__).resolve().parent','root=Path.cwd()'),
                 ('from make_asset_disk import create\nfrom cobalt_native_resource_inventory import capture',
                  'from cobalt_native_resource_inventory import capture\nfrom make_asset_disk import create'),
                 ("str(stage/'elf-named-shm-lifetime-20261008.c')","str(root/'user/elf-named-shm-lifetime.c')")]
        for old,new in changes:
            assert text.count(old)==1,'unexpected named driver change'
            text=text.replace(old,new)
    return text
records=[];mismatches=[]
digest=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
for name in names:
    if not name:continue
    left=main/name;right=trial/name
    is_external=not right.is_file() and name in external
    if is_external:right=external[name]
    if not right.is_file():mismatches.append(dict(file=name,reason='missing trial input'));continue
    record=dict(file=name,main_sha256=digest(left),trial_sha256=digest(right))
    if is_external:
        record['external_test_input']=str(right)
        if name=='tests/test_named_shm_lifetime.py':
            record['reviewed_driver_path_adaptation']=True
            if normalized_named_driver(left)!=normalized_named_driver(right):
                mismatches.append(dict(file=name,reason='unreviewed external driver difference'))
        elif record['main_sha256']!=record['trial_sha256']:
            mismatches.append(dict(file=name,reason='external C fixture differs from main'))
        records.append(record);continue
    if name in expected:
        record['expected_trial_sha256']=digest(expected[name])
        if record['trial_sha256']!=record['expected_trial_sha256']:
            mismatches.append(dict(file=name,reason='trial differs from intended staged candidate'))
    elif record['main_sha256']!=record['trial_sha256']:
        mismatches.append(dict(file=name,reason='unexpected input difference'))
    records.append(record)
helper=trial/'include/vm-ledger.h'
if not helper.is_file() or digest(helper)!=digest(stage/'vm-ledger-20261008.h'):
    mismatches.append(dict(file='include/vm-ledger.h',reason='missing/different helper'))
result=dict(main_head=subprocess.check_output(['git','rev-parse','HEAD'],cwd=main,text=True).strip(),
    trial=str(trial),files_checked=len(records),expected_differences=list(expected)+['include/vm-ledger.h'],
    mismatches=mismatches,records=records,test_completion_claim=False)
output=main/'build/vm-ledger-clean-input-audit-20261008.json'
output.write_text(json.dumps(result,indent=2))
print(json.dumps({k:v for k,v in result.items() if k!='records'},indent=2))
print('EVIDENCE',output)
assert not mismatches,'unexpected candidate inputs; not safe to promote'
