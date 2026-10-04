"""Measure a target-only musl math closure; not a native renderer gate."""
from pathlib import Path
import hashlib
import shutil
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
base = root / 'build/math-evaluation'
archive = base / 'musl-1.2.6.tar.gz'
with archive.open('rb') as stream:
    assert hashlib.file_digest(stream, 'sha256').hexdigest() == 'd585fd3b613c66151fc3249e8ed44f77020cb5e6c1e635a616d3f9f82460512a'
out = Path(tempfile.mkdtemp(prefix='target-', dir=base))
subprocess.run(['tar', '-xzf', str(archive), '-C', str(out)], check=True)
source = out / 'musl-1.2.6'
subprocess.run(['make', '-C', str(source), 'ARCH=x86_64', 'obj/include/bits/alltypes.h'], check=True)
include = subprocess.check_output(['gcc', '-print-file-name=include'], text=True).strip()
flags = ['-m64', '-std=c11', '-O2', '-fPIC', '-ffreestanding', '-fno-builtin',
         '-mno-red-zone', '-fno-stack-protector', '-fno-asynchronous-unwind-tables',
         '-fno-unwind-tables', '-frounding-math', '-ffp-contract=off',
         '-fexcess-precision=standard', '-Wall', '-Werror', '-Wno-unknown-pragmas',
         '-Wno-parentheses', '-Wno-unused-but-set-variable',
         '-nostdinc', '-I' + str(root / 'user/sdk'), '-I' + str(source / 'src/include'),
         '-I' + str(source / 'src/internal'), '-I' + str(source / 'obj/include'),
         '-I' + str(source / 'arch/x86_64'), '-I' + str(source / 'arch/generic'),
         '-I' + str(source / 'include'), '-isystem', include,
         '-D__myos__=1', '-D_XOPEN_SOURCE=700',
         '-U__linux__', '-U__linux', '-Ulinux', '-U__gnu_linux__']
public = 'ceil frexpf sin floorf fma sincosf log10 fmin ceilf trunc fminf cosf sinf atanf atan2f fmaxf fmaf fmax log2f ldexpf rint ldexp floor pow log2 frexp cos logf exp2f powf expf sqrt sqrtf fabs fabsf scalbn scalbnf'.split()
public += 'copysign copysignf llrint llround log lrint lrintf lroundf rintf round roundf truncf'.split()
objects = {}
sources = {}
pending = {name: source / 'src/math' / (name + '.c') for name in public}
pending.update({'fenv': source / 'src/fenv/x86_64/fenv.s',
                'fesetround': source / 'src/fenv/fesetround.c'})
for wave in range(10):
    for name, path in sorted(pending.items()):
        obj = out / (name + '.o')
        extra = ['-Wno-error=maybe-uninitialized'] if path.name == '__rem_pio2_large.c' else []
        subprocess.run(['gcc', *flags, *extra, '-c', str(path), '-o', str(obj)], check=True)
        objects[name] = obj
        sources[name] = path
    symbols = subprocess.check_output(['nm', '-g', *map(str, objects.values())], text=True)
    defined = set()
    undefined = set()
    for line in symbols.splitlines():
        parts = line.split()
        if len(parts) == 2 and parts[0] == 'U':
            undefined.add(parts[1])
        elif len(parts) == 3:
            defined.add(parts[2])
    missing = sorted(undefined - defined)
    print('Target math closure wave', wave, 'unresolved:', missing, flush=True)
    if not missing:
        break
    pending = {}
    for name in missing:
        alternate = {'__pow_log_data': 'pow_data', '__powf_log2_data': 'powf_data',
                     '__rsqrt_tab': 'sqrt_data'}
        candidates = [source / 'src/math' / (name + '.c'),
                      source / 'src/math' / (alternate.get(name, name.removeprefix('__')) + '.c')]
        found = next((path for path in candidates if path.is_file()), None)
        assert found, ('No pure-math provider; do not fake or pull host libc', name)
        assert name not in objects, ('Unresolved after provider compiled', name)
        pending[name] = found
else:
    raise AssertionError('Math closure did not converge')
library = out / 'libmyos-math-prototype.so'
subprocess.run(['ld', '-shared', '--hash-style=both', '-z', 'now', '-z', 'relro',
                '-z', 'noexecstack', '-z', 'defs', '-o', str(library),
                *map(str, objects.values())], check=True)
dynamic = subprocess.check_output(['readelf', '-dW', str(library)], text=True)
assert '(NEEDED)' not in dynamic, dynamic
assert not subprocess.check_output(['nm', '-u', str(library)], text=True).strip()
static_library = out / 'libmyos-math.a'
subprocess.run(['ar', 'rcs', str(static_library), *map(str, objects.values())], check=True)
# Export only math headers, never the musl include tree or OS/pthread types.
sdk_include = out / 'sdk/include'
(sdk_include / 'bits').mkdir(parents=True)
(sdk_include / 'myos').mkdir()
for name in ('features.h', 'fenv.h'):
    shutil.copyfile(source / 'include' / name, sdk_include / name)
shutil.copyfile(source / 'arch/x86_64/bits/fenv.h', sdk_include / 'bits/fenv.h')
shutil.copyfile(root / 'user/sdk/myos/math-types.h', sdk_include / 'myos/math-types.h')
math_header = (source / 'include/math.h').read_text()
assert math_header.count('#include <bits/alltypes.h>') == 1
(sdk_include / 'math.h').write_text(math_header.replace(
    '#include <bits/alltypes.h>', '#include <myos/math-types.h>'))
shutil.copyfile(source / 'COPYRIGHT', out / 'sdk/COPYRIGHT')
print('COMPILE/LINK ONLY: target math bytes', library.stat().st_size,
      'sources', len(sources), 'no undefined/DT_NEEDED; NOT native execution.', flush=True)
print('MATH-ONLY SDK headers:', sdk_include, 'static archive:', static_library, flush=True)
for path in sorted(sources.values()):
    print('SOURCE', path.relative_to(source), flush=True)
