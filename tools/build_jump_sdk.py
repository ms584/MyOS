"""Sealed x86_64 register/stack jump provider; no signal-saving variants."""
from pathlib import Path
import contextlib
import runpy
import shutil
import subprocess
import tempfile
root=Path(__file__).resolve().parents[1]
base=root/'build/jump-evaluation';base.mkdir(parents=True,exist_ok=True)
out=Path(tempfile.mkdtemp(prefix='sdk-',dir=base))
math=globals().get('math_build')
if math is None:
    with (out/'source-build.log').open('w') as log,contextlib.redirect_stdout(log):
        math=runpy.run_path(str(root/'tools/build_math_sdk.py'))
source=math['source'] # Sealed archive verification, not unpinned host asm.
shutil.copyfile(source/'COPYRIGHT',out/'COPYRIGHT');objects=[]
assert (source/'arch/x86_64/bits/setjmp.h').read_text().strip()=='typedef unsigned long __jmp_buf[8];'
for name in ('setjmp','longjmp'):
    assembly=source/'src/setjmp/x86_64'/(name+'.s');obj=out/(name+'.o')
    subprocess.run(['gcc','-m64','-fPIC','-c',str(assembly),'-o',str(obj)],check=True)
    # The original asm has no GNU-stack note; never request executable stack.
    subprocess.run(['objcopy','--add-section','.note.GNU-stack=/dev/null',
                    '--set-section-flags','.note.GNU-stack=noload,readonly',str(obj)],check=True)
    assert not subprocess.check_output(['nm','-u',str(obj)],text=True).strip()
    objects.append(obj)
library=out/'libmyos-jump.a'
subprocess.run(['ar','rcs',str(library),*map(str,objects)],check=True)
print('JUMP SDK COMPILE ONLY:',out,'no unresolved/host OS; native behavior pending tests.',flush=True)
