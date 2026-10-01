"""Check the renderer compiler boundary; no native execution claimed."""
import argparse
from pathlib import Path
import subprocess
import tempfile

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--provider', type=Path, required=True)
options = parser.parse_args()
root = Path(__file__).resolve().parents[1]
intrinsics = subprocess.check_output(['gcc', '-print-file-name=include'], text=True).strip()
provider = options.provider.resolve()
assert provider.is_file()
command = ['python3', str(root / 'tools/renderer_target_compiler.py'), '--language', 'c',
           '--sdk', str(root / 'user/sdk'), '--intrinsics', intrinsics,
           '--provider', str(provider), '--']
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
    result = subprocess.run(command + ['-x', 'c', '-', '-o', str(output)],
                            input=source, text=True, capture_output=True)
    assert result.returncode == 0, result.stderr[-1200:]
    dynamic = subprocess.check_output(['readelf', '-dW', str(output)], text=True)
    assert '(NEEDED)' not in dynamic
    assert not subprocess.check_output(['nm', '-u', str(output)], text=True).strip()
    for forbidden in (['-lm'], ['-L/usr/lib'], ['-Wl,-lm'], ['-Wl,-L,/usr/lib'],
                      ['-Xlinker', '/usr/lib/libc.so'], ['-fopenmp'], ['@response'],
                      ['/usr/lib/libc.a'], ['../../unverified.a']):
        result = subprocess.run(command + forbidden + ['-x', 'c', '-', '-o', str(output)],
                                input=source, text=True, capture_output=True)
        assert result.returncode and 'rejects unverified' in result.stderr, (forbidden, result.stderr)
print('PASS: target runtime link/no undefined/no DT_NEEDED, pinned pthread sizes/no Linux macros; '
      '9 unverified library/search/response/OpenMP routes rejected. NOT native execution.')
