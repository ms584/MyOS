"""Sealed musl div/UTF-8 Cstdlib providers; not a full C++/locale library."""
from pathlib import Path
import hashlib
import re
import shutil
import subprocess
import tempfile
root=Path(__file__).resolve().parents[1]
base=root/'build/cstdlib-evaluation';base.mkdir(parents=True,exist_ok=True)
out=Path(tempfile.mkdtemp(prefix='sdk-',dir=base))
archive=root/'build/math-evaluation/musl-1.2.6.tar.gz'
with archive.open('rb') as stream:
    assert hashlib.file_digest(stream,'sha256').hexdigest()=='d585fd3b613c66151fc3249e8ed44f77020cb5e6c1e635a616d3f9f82460512a'
stdlib=('div','ldiv','lldiv')
utf8=('internal','mblen','mbtowc','mbstowcs','wcstombs','mbsrtowcs','wcsrtombs','wcrtomb','mbrtowc','mbsinit')
members=['COPYRIGHT','include/features.h','include/stdlib.h','src/multibyte/internal.h',
         *['src/stdlib/'+name+'.c' for name in stdlib],
         *['src/multibyte/'+name+'.c' for name in utf8]]
subprocess.run(['tar','-xzf',str(archive),'-C',str(out),*['musl-1.2.6/'+m for m in members]],check=True)
source=out/'musl-1.2.6';shutil.copyfile(source/'COPYRIGHT',out/'COPYRIGHT')
original=re.sub(r'\s+','',(source/'include/stdlib.h').read_text())
target=re.sub(r'\s+','',(root/'user/sdk/stdlib.h').read_text())
for name,integer in (('div_t','int'),('ldiv_t','long'),('lldiv_t','longlong')):
    declaration='typedefstruct{'+integer+'quot,rem;}'+name+';'
    assert declaration in original and declaration in target,(name,'Pinned div ABI mismatch')
shutil.copyfile(source/'include/features.h',out/'features.h')
(out/'locale_impl.h').write_text('/* Fixed UTF-8 MB_CUR_MAX is supplied by the target stdlib.h. */\n')
(out/'internal.h').write_text('#define hidden __attribute__((visibility("hidden")))\n'+
                             (source/'src/multibyte/internal.h').read_text())
flags=['-m64','-std=c11','-Os','-fPIC','-ffreestanding','-fno-builtin','-mno-red-zone',
       '-fno-stack-protector','-fno-asynchronous-unwind-tables','-fno-unwind-tables',
       '-nostdinc','-I'+str(out),'-I'+str(root/'user/sdk'),'-isystem',
       subprocess.check_output(['gcc','-print-file-name=include'],text=True).strip(),
       '-D__myos__=1','-U__linux__','-U__linux','-Ulinux','-U__gnu_linux__',
       '-Wall','-Wextra','-Werror','-Wno-parentheses','-Wno-unused-parameter',
       '-Wno-sign-compare']
objects=[]
paths=[('system',root/'user/platform-cstdlib.c'),
       *[(name,source/'src/stdlib'/(name+'.c')) for name in stdlib],
       *[(name,source/'src/multibyte'/(name+'.c')) for name in utf8]]
for name,path in paths:
    obj=out/(name+'.o')
    local=out/(name+'.c');shutil.copyfile(path,local)
    subprocess.run(['gcc',*flags,'-c',str(local),'-o',str(obj)],check=True)
    objects.append(obj)
symbols=subprocess.check_output(['nm','-g',*map(str,objects)],text=True)
defined=set();undefined=set()
for line in symbols.splitlines():
    parts=line.split()
    if len(parts)==2 and parts[0]=='U':undefined.add(parts[1])
    elif len(parts)==3:defined.add(parts[2])
assert undefined-defined <= {'__errno_location','strlen'},undefined-defined
library=out/'libmyos-cstdlib.a';subprocess.run(['ar','rcs',str(library),*map(str,objects)],check=True)
print('CSTDLIB SDK COMPILE ONLY:',out,'sealed div/UTF-8, no command processor; NOT native proof.',flush=True)
