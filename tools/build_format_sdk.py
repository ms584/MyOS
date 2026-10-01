"""Compile sealed musl formatting with MyOS callback/lock/UTF-8 adapters.

Memory/callback streams only; does not supply file/standard stream lifecycle.
"""
from pathlib import Path
import contextlib
import runpy
import shutil
import subprocess
import tempfile

root=Path(__file__).resolve().parents[1]
base=root/'build/format-evaluation';base.mkdir(parents=True,exist_ok=True)
out=Path(tempfile.mkdtemp(prefix='sdk-',dir=base))
math=globals().get('math_build')
if math is None:
    with (out/'math-build.log').open('w') as log,contextlib.redirect_stdout(log):
        math=runpy.run_path(str(root/'tools/build_math_sdk.py'))
source=math['source'] # The math builder verifies the complete sealed archive.
shutil.copyfile(source/'COPYRIGHT',out/'COPYRIGHT')
shutil.copyfile(root/'user/sdk/myos/stdio_impl.h',out/'stdio_impl.h')
original=(source/'src/stdio/__towrite.c').read_text()
boundary='\nhidden void __towrite_needs_stdio_exit()'
assert original.count(boundary)==1
# Only the pure callback-state transition; not the foreign libc exit hook.
(out/'towrite.c').write_text(original.split(boundary)[0])
original=(source/'src/stdio/fwrite.c').read_text()
assert original.count('size_t k, l = size*nmemb;')==1
(out/'fwrite.c').write_text('#include <errno.h>\n#include <stdint.h>\n'+original.replace(
    'size_t k, l = size*nmemb;',
    'size_t k, l; if (size && nmemb > SIZE_MAX/size) { errno=EOVERFLOW; return 0; } l=size*nmemb;'))
flags=['-m64','-std=c11','-Os','-fPIC','-ffreestanding','-fno-builtin','-mno-red-zone',
       '-fno-stack-protector','-fno-asynchronous-unwind-tables','-fno-unwind-tables',
       '-frounding-math','-ffp-contract=off','-nostdinc','-I'+str(out),
       '-I'+str(root/'user/sdk'),'-I'+str(math['sdk_include']),
       '-isystem',subprocess.check_output(['gcc','-print-file-name=include'],text=True).strip(),
       '-D__myos__=1','-U__linux__','-U__linux','-Ulinux','-U__gnu_linux__',
       '-Wall','-Wextra','-Werror','-Wno-parentheses']
objects=[]
for name,path in [('adapter',root/'user/platform-format.c'),('string',root/'user/platform-sdk-string.c'),
                  ('towrite',out/'towrite.c'),
                  ('fwrite',out/'fwrite.c'),
                  *[(name,source/'src/stdio'/(name+'.c')) for name in
                    ('vfprintf','vsnprintf','snprintf','vasprintf','asprintf')]]:
    extra=['-Wno-error=sign-compare','-Wno-error=implicit-fallthrough',
           '-Wno-error=unterminated-string-initialization','-Wno-error=maybe-uninitialized'] if name=='vfprintf' else []
    if name in ('fwrite','vsnprintf'):extra=['-Wno-error=sign-compare']
    obj=out/(name+'.o');subprocess.run(['gcc',*flags,*extra,'-c',str(path),'-o',str(obj)],check=True)
    objects.append(obj)
for name in ('frexpl','__fpclassifyl','__signbitl'):
    obj=out/(name+'.o')
    subprocess.run(['gcc',*math['flags'],'-c',str(source/'src/math'/(name+'.c')),'-o',str(obj)],check=True)
    objects.append(obj)
symbols=subprocess.check_output(['nm','-g',*map(str,objects),
    *map(str,math['objects'].values())],text=True)
defined=set();undefined=set()
for line in symbols.splitlines():
    parts=line.split()
    if len(parts)==2 and parts[0]=='U':undefined.add(parts[1])
    elif len(parts)==3:defined.add(parts[2])
assert undefined-defined <= {'malloc','free','__errno_location','pthread_mutex_lock','pthread_mutex_unlock'},undefined-defined
library=out/'libmyos-format.a'
subprocess.run(['ar','rcs',str(library),*map(str,objects)],check=True)
print('FORMAT SDK COMPILE ONLY:',out,'callback/memory streams, no file lifecycle; '
      'native behavior unproven until tests pass.',flush=True)
