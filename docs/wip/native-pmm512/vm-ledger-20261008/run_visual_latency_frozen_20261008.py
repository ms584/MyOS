"""Measure actual DOM-pixel acknowledgements on a pinned experimental image.

No kernel builds or guest source changes. This is observation, not acceptance.
"""
from pathlib import Path
import hashlib, os, runpy, sys
root=Path.cwd()
sys.path.insert(0,str(root/'tests'))
import test_system
image=root/'build/gs-errno-3p9yxzo0/frozen.img'
expected='a782ade73f9242c9b288da979b169d05aca76ce267bc6b9e929030424e4f37fc'
assert hashlib.sha256(image.read_bytes()).hexdigest()==expected
OriginalVM=test_system.VM
class FrozenVM(OriginalVM):
    def __init__(self,*args,**kwargs):
        kwargs['boot_image']=image
        super().__init__(*args,**kwargs)
test_system.VM=FrozenVM
os.environ['MYOS_TLS_VISUAL_LATENCY']='1'
os.environ['MYOS_TLS_RESOURCE_ACCOUNTING']='1'
os.environ['MYOS_TLS_LAUNCHES']='1'
print('FROZEN_IMAGE',image,expected,flush=True)
script=Path(sys.argv[1])
sys.argv=[str(script),'/home/xytron404/cd-QJYMwG/build/cobalt-resources-dwxalc2n',
          '/home/xytron404/platform-fonts-01W7Ri/build/platform-fonts-20261007']
try:
    runpy.run_path(str(script),run_name='__main__')
finally:
    assert hashlib.sha256(image.read_bytes()).hexdigest()==expected
