"""Sealed musl pure smoothsort/PRNG, owned guest serialization/finalizers."""
from pathlib import Path
import runpy
import shutil
import subprocess
import tempfile
root=Path(__file__).resolve().parents[1]
base=root/'build/sort-evaluation';base.mkdir(parents=True,exist_ok=True)
out=Path(tempfile.mkdtemp(prefix='sdk-',dir=base))
math=globals().get('math_build') or runpy.run_path(str(root/'tools/build_math_sdk.py'))
source=math['source']
shutil.copyfile(source/'COPYRIGHT',out/'COPYRIGHT')
flags=['-m64','-std=c11','-Os','-fPIC','-ffreestanding','-fno-builtin','-mno-red-zone',
       '-fno-stack-protector','-fno-asynchronous-unwind-tables','-fno-unwind-tables',
       '-nostdinc','-I'+str(root/'user/sdk'),'-isystem',
       subprocess.check_output(['gcc','-print-file-name=include'],text=True).strip(),
       '-Wall','-Wextra','-Werror']
original=(source/'src/stdlib/qsort.c').read_text()
assert original.count('#include "atomic.h"')==1
assert original.count('weak_alias(__qsort_r, qsort_r);')==1
# Only ctz is used, not foreign locks/syscalls. musl x64 fallback gives 0 for 0.
text=original.replace('#include "atomic.h"',
    'static inline int a_ctz_l(unsigned long x) { return x ? __builtin_ctzl(x) : 0; }')
text=text.replace('weak_alias(__qsort_r, qsort_r);',
    'extern __typeof(__qsort_r) qsort_r __attribute__((weak,alias("__qsort_r")));')
assert text.count('width -= l;\n\t}\n}')==1
# Scratch-pointer is unused after cycle; do not leave a dangling local address.
text=text.replace('width -= l;\n\t}\n}', 'width -= l;\n\t}\n\tar[n] = 0;\n}')
(out/'qsort.c').write_text(text)
(out/'qsort_nr.c').write_text('void __qsort_r(void *, __SIZE_TYPE__, __SIZE_TYPE__, '
    'int (*)(const void *,const void *,void *),void *);\n'+(source/'src/stdlib/qsort_nr.c').read_text())
text=(source/'src/prng/rand.c').read_text()
assert text.count('int rand(void)')==1 and text.count('void srand(unsigned s)')==1
(out/'rand.c').write_text(text.replace('int rand(void)','int __myos_rand_unlocked(void)').replace(
    'void srand(unsigned s)','void __myos_srand_unlocked(unsigned s)'))
objects=[]
for name,path in [('qsort',out/'qsort.c'),('qsort_nr',out/'qsort_nr.c'),('rand',out/'rand.c'),
                  ('random',root/'user/platform-random.c'),
                  ('finalize',root/'user/platform-cxx-finalize.c')]:
    obj=out/(name+'.o')
    extra=['-Wno-error=sign-compare'] if name=='qsort' else []
    subprocess.run(['gcc',*flags,*extra,'-c',str(path),'-o',str(obj)],check=True);objects.append(obj)
symbols=subprocess.check_output(['nm','-g',*map(str,objects)],text=True)
defined=set();undefined=set()
for line in symbols.splitlines():
    parts=line.split()
    if len(parts)==2 and parts[0]=='U':undefined.add(parts[1])
    elif len(parts)==3:defined.add(parts[2])
assert undefined-defined <= {'memcpy','malloc','free','pthread_mutex_lock','pthread_mutex_unlock'},undefined-defined
subprocess.run(['ar','rcs',str(out/'libmyos-sort.a'),*map(str,objects)],check=True)
print('SORT SDK COMPILE ONLY:',out,'native behavior unproven until tests pass.',flush=True)
