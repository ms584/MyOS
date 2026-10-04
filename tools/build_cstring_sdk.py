"""Sealed musl bytewise collation helpers, not language-sensitive locale support."""
from pathlib import Path
import hashlib
import shutil
import subprocess
import tempfile
root=Path(__file__).resolve().parents[1]
base=root/'build/cstring-evaluation';base.mkdir(parents=True,exist_ok=True)
out=Path(tempfile.mkdtemp(prefix='sdk-',dir=base))
archive=root/'build/math-evaluation/musl-1.2.6.tar.gz'
with archive.open('rb') as stream:
    assert hashlib.file_digest(stream,'sha256').hexdigest()=='d585fd3b613c66151fc3249e8ed44f77020cb5e6c1e635a616d3f9f82460512a'
subprocess.run(['tar','-xzf',str(archive),'-C',str(out),'musl-1.2.6/COPYRIGHT',
                'musl-1.2.6/src/locale/strcoll.c','musl-1.2.6/src/locale/strxfrm.c'],check=True)
source=out/'musl-1.2.6';shutil.copyfile(source/'COPYRIGHT',out/'COPYRIGHT')
# Extract unchanged pure algorithms, removing only locale_t/current-locale
# plumbing and public _l aliases. Never install a pretend locale.h/API.
for name,signature in (
    ('strcoll','int __strcoll_l(const char *l, const char *r, locale_t loc)'),
    ('strxfrm','size_t __strxfrm_l(char *restrict dest, const char *restrict src, size_t n, locale_t loc)')):
    original=(source/'src/locale'/(name+'.c')).read_text()
    assert original.count(signature)==1
    start=original.index(signature)
    end=original.index('\n'+('int ' if name=='strcoll' else 'size_t ')+name+'(',start)
    body=original[start:end].replace(signature,signature.replace('__'+name+'_l',name).replace(', locale_t loc',''))
    assert 'locale' not in body and 'weak_alias' not in body
    (out/(name+'.c')).write_text('#include <string.h>\n'+body)
flags=['-m64','-std=c11','-Os','-fPIC','-ffreestanding','-fno-builtin','-mno-red-zone',
       '-fno-stack-protector','-fno-asynchronous-unwind-tables','-fno-unwind-tables',
       '-nostdinc','-I'+str(root/'user/sdk'),'-isystem',subprocess.check_output(['gcc','-print-file-name=include'],text=True).strip(),
       '-D__myos__=1','-U__linux__','-U__linux','-Ulinux','-U__gnu_linux__',
       '-Wall','-Wextra','-Werror']
objects=[]
for name,path in [('strcoll',out/'strcoll.c'),('strxfrm',out/'strxfrm.c'),
                  ('primitives',root/'user/platform-sdk-string.c')]:
    obj=out/(name+'.o');subprocess.run(['gcc',*flags,'-c',str(path),'-o',str(obj)],check=True);objects.append(obj)
library=out/'libmyos-cstring.a';subprocess.run(['ar','rcs',str(library),*map(str,objects)],check=True)
print('CSTRING SDK COMPILE ONLY:',out,'sealed bytewise profile, NOT locale/native proof.',flush=True)
