"""Build a sealed pure calendar primitive, not musl timezone/libc/syscalls."""
from pathlib import Path
import hashlib
import subprocess
import tempfile

root=Path(__file__).resolve().parents[1]
archive=root/'build/math-evaluation/musl-1.2.6.tar.gz'
with archive.open('rb') as stream:
    assert hashlib.file_digest(stream,'sha256').hexdigest()=='d585fd3b613c66151fc3249e8ed44f77020cb5e6c1e635a616d3f9f82460512a'
base=root/'build/time-evaluation';base.mkdir(parents=True,exist_ok=True)
out=Path(tempfile.mkdtemp(prefix='sdk-',dir=base))
subprocess.run(['tar','-xzf',str(archive),'-C',str(out),
                'musl-1.2.6/src/time/__secs_to_tm.c','musl-1.2.6/COPYRIGHT'],check=True)
original=out/'musl-1.2.6/src/time/__secs_to_tm.c'
content=original.read_text()
assert content.count('#include "time_impl.h"')==1 and content.count('__secs_to_tm')==1
source=out/'secs-to-tm.c'
source.write_text(content.replace('#include "time_impl.h"','#include <time.h>').replace(
    '__secs_to_tm','__myos_secs_to_tm'))
include=subprocess.check_output(['gcc','-print-file-name=include'],text=True).strip()
flags=['-m64','-std=c11','-Os','-fPIC','-ffreestanding','-fno-builtin','-mno-red-zone',
       '-fno-stack-protector','-fno-asynchronous-unwind-tables','-fno-unwind-tables',
       '-nostdinc','-I'+str(root/'user/sdk'),'-isystem',include,
       '-D__myos__=1','-U__linux__','-U__linux','-Ulinux','-U__gnu_linux__',
       '-Wall','-Wextra','-Werror']
objects=[]
for name,path in (('calendar',root/'user/platform-calendar.c'),('convert',source)):
    obj=out/(name+'.o')
    subprocess.run(['gcc',*flags,'-c',str(path),'-o',str(obj)],check=True)
    objects.append(obj)
library=out/'libmyos-calendar.a'
subprocess.run(['ar','rcs',str(library),*map(str,objects)],check=True)
print('UTC calendar SDK:',out,'pure converter + errno runtime dependency; '
      'NOT timezone database/DST or native execution.',flush=True)
