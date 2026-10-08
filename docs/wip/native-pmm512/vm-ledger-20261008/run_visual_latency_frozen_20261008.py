"""Measure actual DOM-pixel acknowledgements on a pinned experimental image.

No kernel builds or guest source changes. This is observation, not acceptance.
"""
from pathlib import Path
import hashlib, os, runpy, sys, struct, tempfile
root=Path.cwd()
sys.path.insert(0,str(root/'tests'))
import test_system
image=root/'build/gs-errno-3p9yxzo0/frozen.img'
expected='a782ade73f9242c9b288da979b169d05aca76ce267bc6b9e929030424e4f37fc'
assert hashlib.sha256(image.read_bytes()).hexdigest()==expected
# Extract only the exact packaged runtime from the successful combo launch,
# never its CA keys. Loader still verifies its embedded build identity.
source=root/'build/native-verified-tls-mrm5_amc/assets.img'
with source.open('rb') as inp:
    header=inp.read(48)
    magic,version,count,total=struct.unpack('<16sIIQ',header[:32])
    offset,size=struct.unpack('<QQ',header[32:])
    assert magic.rstrip(b'\0')==b'MYOS-ASSET-v2' and version==2
    assert 16<=size<=65536+512+16 and 48<=offset<=total-size
    inp.seek(offset);runtime=inp.read(size);assert len(runtime)==size
out=Path(tempfile.mkdtemp(prefix='visual-latency-input-',dir=root/'build'))
packed=out/'platform-runtime.mrt'
with packed.open('xb') as output:output.write(runtime)
os.environ['MYOS_TLS_FROZEN_RUNTIME']=str(packed)
print('FROZEN_RUNTIME',packed,hashlib.sha256(runtime).hexdigest(),flush=True)
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
