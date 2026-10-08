"""Existing TLS/signal contracts on hash-pinned GS+ledger guest images.

Guest source/build remains untouched; only host VM boot_image is overridden.
"""
from pathlib import Path
import hashlib,json,runpy,shutil,sys,tempfile
root=Path.cwd();sys.path.insert(0,str(root/'tests'));sys.path.insert(0,str(root/'tools'))
import test_system
out=Path(tempfile.mkdtemp(prefix='gs-tls-signal-',dir=root/'build'))
sources={False:root/'build/gs-errno-3p9yxzo0/frozen.img',True:root/'build/gs-errno-mybnvp11/frozen.img'}
images={};hashes={}
for uefi,source in sources.items():
 image=out/('uefi.img' if uefi else 'bios.img');shutil.copyfile(source,image)
 images[uefi]=image;hashes[str(uefi)]=hashlib.sha256(image.read_bytes()).hexdigest()
(out/'inputs.json').write_text(json.dumps(hashes,indent=2))
OriginalVM=test_system.VM
active_uefi=False
class FrozenVM(OriginalVM):
 def __init__(self,*args,**kwargs):
  kwargs['boot_image']=images[active_uefi]
  super().__init__(*args,**kwargs)
test_system.VM=FrozenVM
import test_native_tls
try:
 for uefi in (False,True):
  active_uefi=uefi;test_native_tls.main(uefi=uefi)
  shutil.copyfile(root/'build/system-tests'/('native-tls-uefi.log' if uefi else 'native-tls.log'),out/('tls-uefi.log' if uefi else 'tls-bios.log'))
  sys.argv=['tests/test_cross_signal.py','user/elf-cross-signal.c']+(['--uefi'] if uefi else [])
  runpy.run_path(str(root/'tests/test_cross_signal.py'),run_name='__main__')
  shutil.copyfile(root/'build/system-tests'/('cross-signals-uefi.log' if uefi else 'cross-signals.log'),out/('signals-uefi.log' if uefi else 'signals-bios.log'))
finally:
 for uefi,image in images.items():assert hashlib.sha256(image.read_bytes()).hexdigest()==hashes[str(uefi)]
 print('EVIDENCE',out,flush=True)
