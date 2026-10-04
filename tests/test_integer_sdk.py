"""Verify target integer limits and every supplied C/C++ format macro.

Compile evidence, not a claim of runtime printf/scanf support.
"""
from pathlib import Path
import argparse
import subprocess
import tempfile

root=Path(__file__).resolve().parents[1]
parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--hosted',action='store_true',help='Target hosted language profile, no host headers')
options=parser.parse_args()
base=root/'build/integer-evaluation';base.mkdir(parents=True,exist_ok=True)
out=Path(tempfile.mkdtemp(prefix='formats-',dir=base))
include=subprocess.check_output(['gcc','-print-file-name=include'],text=True).strip()
flags=['-m64','-ffreestanding','-fno-builtin','-nostdinc','-isystem',include,
       '-I'+str(root/'user/sdk'),'-D__myos__=1','-U__linux__','-U__linux',
       '-Ulinux','-U__gnu_linux__','-Wall','-Wextra','-Wformat=2','-Werror']
if options.hosted:
    flags.remove('-ffreestanding')
body='''#include <limits.h>
#include <inttypes.h>
#include <assert.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
static_assert(sizeof(long)==8,"LP64");
char *(*environment_reader)(const char *)=getenv;
gid_t (*group_reader)(void)=getegid;
#ifdef __linux__
#error Host ABI
#endif
#if CHAR_BIT != 8 || SCHAR_MAX != 127 || UCHAR_MAX != 255 || SHRT_MAX != 32767
#error Bad small integer limits
#endif
#if INT_MAX != 2147483647 || UINT_MAX != 4294967295U
#error Bad int limits
#endif
#if LONG_MAX != INT64_MAX || LONG_MIN != INT64_MIN || ULONG_MAX != UINT64_MAX
#error Bad LP64 limits
#endif
#if LLONG_MAX != INT64_MAX || LLONG_MIN != INT64_MIN || ULLONG_MAX != UINT64_MAX
#error Bad long long limits
#endif
#if (CHAR_MIN == 0) != EXPECT_UNSIGNED_CHAR
#error Plain char sign mismatch
#endif
extern int print_probe(const char*,...) __attribute__((format(printf,1,2)));
extern int scan_probe(const char*,...) __attribute__((format(scanf,1,2)));
void test_formats(void) {
'''
for suffix,signed,unsigned in (
    ('8','int8_t','uint8_t'),('16','int16_t','uint16_t'),
    ('32','int32_t','uint32_t'),('64','int64_t','uint64_t'),
    *[(kind+str(width),'int_'+kind.lower()+str(width)+'_t',
       'uint_'+kind.lower()+str(width)+'_t') for kind in ('LEAST','FAST') for width in (8,16,32,64)],
    ('MAX','intmax_t','uintmax_t'),('PTR','intptr_t','uintptr_t')):
    body+='{ '+signed+' s=0; '+unsigned+' u=0;\n'
    for conversion in ('d','i','o','u','x','X'):
        var='s' if conversion in ('d','i') else 'u'
        body+=f'print_probe("%" PRI{conversion}{suffix}, {var});\n'
    for conversion in ('d','i','o','u','x'):
        var='s' if conversion in ('d','i') else 'u'
        body+=f'scan_probe("%" SCN{conversion}{suffix}, &{var});\n'
    body+='}\n'
body+='}\n'
for compiler,language in (('gcc','c'),('g++','c++')):
    for unsigned in (False,True):
        source=out/(compiler+('-unsigned' if unsigned else '-signed')+'.'+('c' if language=='c' else 'cpp'))
        source.write_text(body)
        subprocess.run([compiler,*flags,*(['-nostdinc++'] if language=='c++' else []),
            '-std=c++17' if language=='c++' else '-std=c11',
            '-funsigned-char' if unsigned else '-fsigned-char',
            '-DEXPECT_UNSIGNED_CHAR='+str(int(unsigned)),'-fsyntax-only',str(source)],check=True)
print('PASS: all 154 integer format macros match target C/C++ types; LP64 limits '
      'and both plain-char modes. NOT runtime formatting/scanning.',out,flush=True)
