"""Canonical errno ABI header checks; constants are not API implementation proof."""
import argparse
import hashlib
from pathlib import Path
import re
import subprocess
import tempfile
import tarfile

root=Path(__file__).resolve().parents[1]
parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--reference',type=Path,required=True)
parser.add_argument('--configuration',type=Path,required=True)
args=parser.parse_args()
reference=args.reference.resolve()
assert reference.is_relative_to((root/'build/math-evaluation').resolve())
assert reference.parts[-5:]==('musl-1.2.6','arch','generic','bits','errno.h')
archive=root/'build/math-evaluation/musl-1.2.6.tar.gz'
with archive.open('rb') as stream:
    assert hashlib.file_digest(stream,'sha256').hexdigest()==(
        'd585fd3b613c66151fc3249e8ed44f77020cb5e6c1e635a616d3f9f82460512a')
with tarfile.open(archive) as sealed:
    assert reference.read_bytes()==sealed.extractfile(
        'musl-1.2.6/arch/generic/bits/errno.h').read()
expected=dict(re.findall(r'^#define\s+(E\w+)\s+(\w+)\s*$',reference.read_text(),re.M))
assert len(expected)>130
cfg=args.configuration.resolve()
assert cfg.is_relative_to((root/'build/cxx-target-evaluation').resolve())
base=root/'build/errno-evaluation';base.mkdir(parents=True,exist_ok=True)
intrinsics=subprocess.check_output(['gcc','-print-file-name=include'],text=True).strip()
flags=['-m64','-nostdinc','-isystem',intrinsics,'-I'+str(root/'user/sdk')]
for language,standard in [('c','c11'),('c++','c++98'),('c++','c++17')]:
    command=['gcc' if language=='c' else 'g++',*flags,'-std='+standard]
    if language=='c++':command+=['-nostdinc++']
    result=subprocess.check_output([*command,'-dM','-E','-x',language,'-'],
                                   input='#include <errno.h>\n',text=True)
    actual=dict(re.findall(r'^#define\s+(E\w+)\s+(\w+)\s*$',result,re.M))
    assert actual==expected,(standard,actual.keys()^expected.keys())
with tempfile.TemporaryDirectory(prefix='header-',dir=base) as scratch:
    source='#include <cerrno>\n#include <bits/error_constants.h>\n'
    source+='static_assert((int)std::errc::address_in_use==98,"errno ABI");\n'
    source+='int *check(){return &errno;}\n'
    subprocess.run(['g++','-m64','-std=c++17','-nostdinc','-nostdinc++',
                    '-isystem',intrinsics,'-isystem',str(cfg/'build/include'),
                    '-isystem',str(cfg/'build/include/x86_64-unknown-myos'),
                    '-idirafter',str(root/'user/sdk'),'-Wall','-Wextra','-Werror',
                    '-x','c++','-','-c','-o',str(Path(scratch)/'errno.o')],
                   input=source,text=True,check=True)
print('PASS: all canonical musl errno macros/aliases C11/C++98/17 and actual GCC errc; NOT all APIs.')
