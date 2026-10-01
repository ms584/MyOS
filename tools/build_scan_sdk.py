"""Sealed pure strto*/memory scanf/UTF8 providers; not a foreign FILE ABI."""
from pathlib import Path
import contextlib
import runpy
import shutil
import subprocess
import tempfile
root=Path(__file__).resolve().parents[1]
base=root/'build/scan-evaluation';base.mkdir(parents=True,exist_ok=True)
out=Path(tempfile.mkdtemp(prefix='sdk-',dir=base))
math=globals().get('math_build')
if math is None:
    with (out/'math-build.log').open('w') as log,contextlib.redirect_stdout(log):
        math=runpy.run_path(str(root/'tools/build_math_sdk.py'))
source=math['source']  # Hash-verified musl archive and retained source/license.
shutil.copyfile(source/'COPYRIGHT',out/'COPYRIGHT')
for name in ('stdio.h','stdio_impl.h'):
    shutil.copyfile(root/'user/sdk/myos/scan-stdio.h',out/name)
for name in ('intscan.h','floatscan.h','shgetc.h'):
    shutil.copyfile(source/'src/internal'/name,out/name)
utf8_header=(source/'src/multibyte/internal.h').read_text()
(out/'internal.h').write_text('#define hidden __attribute__((visibility("hidden")))\n'+utf8_header)
# UTF8-only guest profile, not the original libc's global locale/thread state.
(out/'locale_impl.h').write_text('#define MB_CUR_MAX 4\n')
paths=[('memory-adapter',root/'user/platform-memory-scan.c'),
       *[(name,source/'src/internal'/(name+'.c')) for name in ('shgetc','intscan','floatscan')],
       *[(name,source/'src/stdlib'/(name+'.c')) for name in
         ('strtol','strtod','atoi','atol','atoll','atof','abs','labs','llabs','bsearch')],
       *[(name,source/'src/stdio'/(name+'.c')) for name in ('vfscanf','vsscanf','sscanf')],
       *[(name,source/'src/multibyte'/(name+'.c')) for name in ('internal','mbrtowc','mbsinit')]]
flags=['-m64','-std=c11','-O2','-fPIC','-ffreestanding','-fno-builtin','-mno-red-zone',
       '-fno-stack-protector','-fno-asynchronous-unwind-tables','-fno-unwind-tables',
       '-frounding-math','-ffp-contract=off','-fexcess-precision=standard',
       '-nostdinc','-I'+str(out),'-I'+str(root/'user/sdk'),'-I'+str(math['sdk_include']),
       '-isystem',subprocess.check_output(['gcc','-print-file-name=include'],text=True).strip(),
       '-D__myos__=1','-U__linux__','-U__linux','-Ulinux','-U__gnu_linux__',
       '-Wall','-Wextra','-Werror','-Wno-parentheses']
objects=[]
def compile_target(arguments):
    result=subprocess.run(arguments,text=True,capture_output=True)
    with (out/'compile.log').open('a') as log:
        log.write(repr(arguments)+'\n'+result.stdout+result.stderr+'\n')
    if result.returncode:
        print('\n'.join(line for line in result.stderr.splitlines() if 'error:' in line),flush=True)
        raise RuntimeError(('Target compiler failure; full diagnostic retained',out/'compile.log'))
for name,path in paths:
    local=out/(name+'.c');text=path.read_text()
    if name=='vfscanf':
        assert text.count('#include <wctype.h>')==1
        assert text.count('weak_alias(vfscanf,__isoc99_vfscanf);')==1
        # This unit never calls wctype. Its FILE is private memory-only, so
        # never export a public vfscanf/isoc99_vfscanf with the wrong layout.
        text=text.replace('#include <wctype.h>','').replace('weak_alias(vfscanf,__isoc99_vfscanf);','')
        assert text.count('char *s;')==1 and text.count('wchar_t *wcs;')==1
        text=text.replace('char *s;','char *s = 0;').replace('wchar_t *wcs;','wchar_t *wcs = 0;')
    local.write_text(text)
    obj=out/(name+'.o')
    extra=[] if name=='memory-adapter' else ['-Wno-error=sign-compare','-Wno-error=implicit-fallthrough']
    compile_target(['gcc',*flags,*extra,'-c',str(local),'-o',str(obj)])
    objects.append(obj)
for name in ('copysignl','scalbnl','fmodl','fabsl','__fpclassifyl'):
    obj=out/(name+'.o')
    compile_target(['gcc',*math['flags'],'-c',str(source/'src/math'/(name+'.c')),'-o',str(obj)])
    objects.append(obj)
symbols=subprocess.check_output(['nm','-g',*map(str,objects),*map(str,math['objects'].values())],text=True)
defined=set();undefined=set()
for line in symbols.splitlines():
    parts=line.split()
    if len(parts)==2 and parts[0]=='U':undefined.add(parts[1])
    elif len(parts)==3:defined.add(parts[2])
assert undefined-defined <= {'malloc','realloc','free','__errno_location','memset','memcpy','memchr'},undefined-defined
library=out/'libmyos-scan.a'
subprocess.run(['ar','rcs',str(library),*map(str,objects)],check=True)
print('MEMORY SCAN SDK COMPILE ONLY:',out,'sealed numeric/UTF8 parsing; NOT native evidence or public FILE ABI.',flush=True)
