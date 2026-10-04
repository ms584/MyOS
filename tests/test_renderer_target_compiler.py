"""Check the renderer compiler boundary; no native execution claimed."""
import argparse
from pathlib import Path
import subprocess
import tempfile

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--provider', type=Path, required=True)
parser.add_argument('--math-include', type=Path)
parser.add_argument('--environment', action='store_true', help='Require the real guest environment SDK')
parser.add_argument('--format', action='store_true', help='Require actual target memory-formatting providers')
parser.add_argument('--cxx-headers', type=Path, help='Verified source libsupc++ directory for header-order check')
options = parser.parse_args()
root = Path(__file__).resolve().parents[1]
intrinsics = subprocess.check_output(['gcc', '-print-file-name=include'], text=True).strip()
provider = options.provider.resolve()
assert provider.is_file()
command = ['python3', str(root / 'tools/renderer_target_compiler.py'), '--language', 'c',
           '--sdk', str(root / 'user/sdk'), '--intrinsics', intrinsics,
           '--provider', str(provider)]
if options.math_include:
    command += ['--math-include', str(options.math_include.resolve())]
command += ['--']
with tempfile.TemporaryDirectory(prefix='boundary-', dir=provider.parent) as scratch:
    output = Path(scratch) / 'probe'
    source = '''#include <pthread.h>
#include <time.h>
#if defined(__linux__) || defined(__GLIBC__)
#error Linux ABI leakage
#endif
_Static_assert(sizeof(pthread_mutex_t)==80, "mutex");
_Static_assert(sizeof(pthread_cond_t)==96, "cond");
int main(void) { struct timespec t; return clock_gettime(CLOCK_MONOTONIC,&t); }
'''
    if options.math_include:
        source = source.replace('#include <time.h>', '#include <time.h>\n#include <math.h>\n'
                                '_Static_assert(sizeof(float_t)==4 && sizeof(double_t)==8, "math evaluation types");')
        source = source.replace('return clock_gettime(CLOCK_MONOTONIC,&t);',
                                'double (*volatile f)(double)=sqrt;\n'
                                'return clock_gettime(CLOCK_MONOTONIC,&t)+(int)f(4.0);')
    if options.environment:
        source = source.replace('#include <time.h>', '#include <time.h>\n#include <stdlib.h>\n#include <unistd.h>')
        source = source.replace('clock_gettime(CLOCK_MONOTONIC,&t)',
                                '(clock_gettime(CLOCK_MONOTONIC,&t)+getgid()+getegid()+!!getenv("MYOS_BOUNDARY_PROBE"))')
    if options.format:
        source = source.replace('#include <time.h>', '#include <time.h>\n#include <stdio.h>')
        source = source.replace('struct timespec t;',
                                'struct timespec t; char buffer[32];\n'
                                'if(snprintf(buffer,sizeof(buffer),"%ld:%.2Lf",17L,1.25L)!=7) return 9;')
    for language in ('c', 'cpp'):
        current = command.copy();current[3] = language
        text = source if language == 'c' else source.replace('_Static_assert', 'static_assert')
        x = 'c' if language == 'c' else 'c++'
        result = subprocess.run(current + ['-x', x, '-', '-o', str(output)],
                                input=text, text=True, capture_output=True)
        assert result.returncode == 0, result.stderr[-1200:]
        dynamic = subprocess.check_output(['readelf', '-dW', str(output)], text=True)
        assert '(NEEDED)' not in dynamic
        assert not subprocess.check_output(['nm', '-u', str(output)], text=True).strip()
        for forbidden in (['-lm'], ['-L/usr/lib'], ['-Wl,-lm'], ['-Wl,-L,/usr/lib'],
                          ['-Xlinker', '/usr/lib/libc.so'], ['-fopenmp'], ['@response'],
                          ['/usr/lib/libc.a'], ['../../unverified.a']):
            result = subprocess.run(current + forbidden + ['-x', x, '-', '-o', str(output)],
                                    input=text, text=True, capture_output=True)
            assert result.returncode and 'rejects unverified' in result.stderr, (forbidden, result.stderr)
    if options.cxx_headers:
        headers=options.cxx_headers.resolve()
        assert headers.is_relative_to((root/'build/cxx-target-evaluation').resolve())
        current=command.copy();current[3]='cpp'
        current=current[:-1]+['--cxx-library','--']
        current += ['-I'+str(headers), '-I'+str(headers.parents[2]/'build/include'),
                    '-I'+str(headers.parents[2]/'build/include/x86_64-unknown-myos')]
        source='''#include <new>
#include <pthread.h>
#if !__STDC_HOSTED__ || defined(__linux__) || defined(__GLIBC__)
#error Wrong C++ language/target profile
#endif
static_assert(sizeof(pthread_mutex_t)==80 && sizeof(pthread_cond_t)==96);
static_assert(sizeof(std::nothrow_t)>0);
static_assert(sizeof(std::bad_alloc)>0); // Upstream <new>, not the smaller SDK shim.
extern "C" int main() { return 0; }
'''
        result=subprocess.run(current+['-std=c++17','-x','c++','-','-o',str(output)],
                              input=source,text=True,capture_output=True)
        assert result.returncode==0,result.stderr[-1800:]
        assert '(NEEDED)' not in subprocess.check_output(['readelf','-dW',str(output)],text=True)
        assert not subprocess.check_output(['nm','-u',str(output)],text=True).strip()
        for forbidden in ('-lstdc++','-L/usr/lib','-Wl,-lstdc++'):
            result=subprocess.run(current+[forbidden,'-x','c++','-','-o',str(output)],
                                  input=source,text=True,capture_output=True)
            assert result.returncode and 'rejects unverified' in result.stderr
print('PASS: target runtime link/no undefined/no DT_NEEDED, pinned pthread sizes/no Linux macros; '
      'C/C++ math-header/provider link when requested; 9 unverified library/search/response/OpenMP '
      'routes rejected in each language. NOT native execution.')
