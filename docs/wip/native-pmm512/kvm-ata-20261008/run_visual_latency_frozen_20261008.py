"""Measure actual DOM-pixel acknowledgements on a pinned experimental image.

No kernel builds or guest source changes. This is observation, not acceptance.
"""
from pathlib import Path
import hashlib, os, runpy, sys, struct, tempfile, types
root=Path.cwd()
sys.path.insert(0,str(root/'tests'))
import test_system
if os.environ.get('MYOS_TLS_EXPLICIT_KVM') == '1':
    from qemu_kvm_optin_20261008 import install
    install()
image=root/'build/gs-errno-3p9yxzo0/frozen.img'
expected='a782ade73f9242c9b288da979b169d05aca76ce267bc6b9e929030424e4f37fc'
if len(sys.argv)==4:
    image=Path(sys.argv[2]).resolve();expected=sys.argv[3]
    assert image.is_relative_to((root/'build').resolve())
    assert len(expected)==64 and all(c in '0123456789abcdef' for c in expected)
else:assert len(sys.argv)==2
assert hashlib.sha256(image.read_bytes()).hexdigest()==expected
# Extract only the exact packaged runtime from the successful combo launch,
# never its CA keys. Loader still verifies its embedded build identity.
source=Path(os.environ.get('MYOS_TLS_RUNTIME_ASSET_SOURCE',
    str(root/'build/native-verified-tls-mrm5_amc/assets.img'))).resolve()
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
if os.environ.get('MYOS_TLS_RAW_PC')=='1':
    observer=runpy.run_path(str(Path(__file__).with_name('cobalt_raw_pc_observer_20261008.py')))['Observer']
    module=types.ModuleType('cobalt_gpu_frame_observer');module.Observer=observer
    sys.modules[module.__name__]=module
    os.environ['MYOS_TLS_GPU_FRAMES']='1'
os.environ['MYOS_TLS_VISUAL_LATENCY']='1'
# Current kernel.elf is not the frozen image's matching ELF. Do not read
# kernel-private inventory by addresses obtained from an unrelated nm output.
os.environ['MYOS_TLS_RESOURCE_ACCOUNTING']='0'
os.environ.setdefault('MYOS_TLS_LAUNCHES','1')
print('FROZEN_IMAGE',image,expected,flush=True)
script=Path(sys.argv[1])
sys.argv=[str(script),'/home/xytron404/cd-QJYMwG/build/cobalt-resources-dwxalc2n',
          '/home/xytron404/platform-fonts-01W7Ri/build/platform-fonts-20261007']
try:
    runpy.run_path(str(script),run_name='__main__')
finally:
    assert hashlib.sha256(image.read_bytes()).hexdigest()==expected
