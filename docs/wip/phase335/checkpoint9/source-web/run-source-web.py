from pathlib import Path
import os,sys,runpy
root=Path('/home/xytron404/myos')
private=root/'build/phase335-abort-worktree'
assert Path.cwd()==private
os.environ['MYOS_SOURCE_CORE_MANIFEST']=str(root/'build/phase335-core-workspace/core-build-identity.json')
os.environ['MYOS_RENDERER_PROVIDER_PATH']=str(private/'build/renderer-provider-evaluation/phase335-source-vp9')
for key in ['MYOS_TLS_TASK_SNAPSHOT','MYOS_TLS_RESOURCE_DIAGNOSTIC','MYOS_TLS_CPU_ACCOUNTING','MYOS_TLS_GPU_FRAMES','MYOS_TLS_PRESENT_TIMING']:
    os.environ.pop(key,None)
sys.path.insert(0,str(private/'tests'))
sys.path.insert(0,str(private/'docs/wip/native-pmm512/kvm-ata-20261008'))
from qemu_kvm_optin_20261008 import install
install()
sys.argv=['tests/test_cobalt_native_acceptance.py','/home/xytron404/cd-QJYMwG/build/cobalt-resources-dwxalc2n','/home/xytron404/platform-fonts-01W7Ri/build/platform-fonts-20261007']
runpy.run_path('tests/test_cobalt_native_acceptance.py',run_name='__main__')
