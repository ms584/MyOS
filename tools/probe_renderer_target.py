"""Configure pristine Mesa against MyOS, retaining honest target failures.

Does not build a native graphics backend or modify shared boot images.
Generated cross files, compiler probes and logs are isolated build artifacts.
"""
from pathlib import Path
import argparse
import contextlib
import hashlib
import json
import os
import runpy
import shutil
import subprocess
import tempfile

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--compile', action='store_true', help='Also collect actual target compile failures')
parser.add_argument('--keep-going', action='store_true', help='Collect all remaining compile failures instead of stopping after eight units')
parser.add_argument('--cxx-configuration', type=Path,
                    help='Real generated GCC MyOS headers and native-tested no-exception container SDK')
options = parser.parse_args()
root = Path(__file__).resolve().parents[1]
base = root / 'build/renderer-evaluation'
archive = base / 'mesa-25.0.7.tar.xz'
with archive.open('rb') as stream:
    assert hashlib.file_digest(stream, 'sha256').hexdigest() == '592272df3cf01e85e7db300c449df5061092574d099da275d19e97ef0510f8a6'
out = Path(tempfile.mkdtemp(prefix='target-config-', dir=base))
subprocess.run(['tar', '-xJf', str(archive), '-C', str(out)], check=True)
source = out / 'mesa-25.0.7'
# The C11 patch deliberately has a basename-only path for fixture copies.
patch = (root / 'tools/mesa_c11_target.patch').read_text().replace(
    'a/threads_posix.c', 'a/src/c11/impl/threads_posix.c').replace(
    'b/threads_posix.c', 'b/src/c11/impl/threads_posix.c')
for content in (patch, (root / 'tools/mesa_thread_util_target.patch').read_text(),
                (root / 'tools/mesa_build_target.patch').read_text(),
                (root / 'tools/mesa_file_target.patch').read_text(),
                (root / 'tools/mesa_optional_target.patch').read_text(),
                (root / 'tools/mesa_memory_target.patch').read_text(),
                (root / 'tools/mesa_sort_target.patch').read_text()):
    subprocess.run(['git', 'apply', '--check', '--directory=' + str(source.relative_to(root)), '-'],
                   input=content, text=True, cwd=root, check=True)
    subprocess.run(['git', 'apply', '--directory=' + str(source.relative_to(root)), '-'],
                   input=content, text=True, cwd=root, check=True)
for check in (True,False):
    subprocess.run(['git','apply',*(['--check'] if check else []),
                    '--directory='+str(source.relative_to(root)),str(root/'tools/mesa_time_target.patch')],
                   cwd=root,check=True)
shutil.copyfile(root/'user/platform-mesa-time.c',source/'src/util/myos_time.c')
intrinsics = subprocess.check_output(['gcc', '-print-file-name=include'], text=True).strip()
flags = ['-m64', '-Oz', '-fPIC', '-ffreestanding', '-fno-builtin', '-mno-red-zone',
         '-fno-stack-protector', '-fno-asynchronous-unwind-tables', '-fno-unwind-tables',
         '-nostdinc', '-isystem', intrinsics, '-I' + str(root / 'user/sdk')]
runtime = out / 'runtime.o'
process_exit = out / 'process-exit.o'
tls = out / 'tls.o'
string = out / 'string.o'
sleep = out / 'sleep.o'
environment = out / 'environment.o'
subprocess.run(['gcc', *flags, '-c', str(root / 'user/platform-runtime.c'), '-o', str(runtime)], check=True)
subprocess.run(['gcc', *flags, '-c', str(root / 'user/platform-exit.c'), '-o', str(process_exit)], check=True)
subprocess.run(['nasm', '-f', 'elf64', str(root / 'user/platform-tls.asm'), '-o', str(tls)], check=True)
subprocess.run(['gcc', *flags, '-c', str(root / 'user/platform-sdk-string.c'), '-o', str(string)], check=True)
subprocess.run(['gcc', *flags, '-c', str(root / 'user/platform-environment.c'), '-o', str(environment)], check=True)
subprocess.run(['gcc', *flags, '-DMYOS_SDK_EXTERNAL_C23_TIME=1', '-c',
                str(root/'user/platform-c11-time.c'), '-o',str(sleep)],check=True)
with (out/'calendar-build.log').open('w') as calendar_log, contextlib.redirect_stdout(calendar_log):
    calendar_build=runpy.run_path(str(root/'tools/build_time_sdk.py'))
provider = out / 'libmyos-runtime-probe.a'
with (out / 'math-build.log').open('w') as math_log, contextlib.redirect_stdout(math_log):
    math_build = runpy.run_path(str(root / 'tools/build_math_sdk.py'))
with (out / 'format-build.log').open('w') as format_log, contextlib.redirect_stdout(format_log):
    format_build = runpy.run_path(str(root / 'tools/build_format_sdk.py'),init_globals={'math_build':math_build})
with (out / 'sort-build.log').open('w') as sort_log, contextlib.redirect_stdout(sort_log):
    sort_build = runpy.run_path(str(root / 'tools/build_sort_sdk.py'),init_globals={'math_build':math_build})
with (out / 'scan-build.log').open('w') as scan_log, contextlib.redirect_stdout(scan_log):
    scan_build = runpy.run_path(str(root / 'tools/build_scan_sdk.py'),init_globals={'math_build':math_build})
with (out / 'jump-build.log').open('w') as jump_log, contextlib.redirect_stdout(jump_log):
    jump_build = runpy.run_path(str(root / 'tools/build_jump_sdk.py'),init_globals={'math_build':math_build})
math_include = out / 'math-include'
shutil.copytree(math_build['sdk_include'], math_include)
subprocess.run(['ar', 'rcs', str(provider), str(runtime), str(tls), str(string), str(sleep),
                str(environment), str(process_exit),
                *map(str,format_build['objects']),
                *map(str,sort_build['objects']),
                *map(str,scan_build['objects']),
                *map(str,jump_build['objects']),
                *map(str,calendar_build['objects']),
                *map(str, math_build['objects'].values())], check=True)
if options.cxx_configuration:
    from build_cxx_containers_sdk import build as build_containers
    cxx_configuration=options.cxx_configuration.resolve()
    container_build=build_containers(cxx_configuration,provider)
    # Pure string/math objects already belong to this checked provider.
    subprocess.run(['ar','rcs',str(provider),*map(str,container_build['cxx_objects'])],check=True)
compiler = root / 'tools/renderer_target_compiler.py'
def command(language):
    result=['python3', str(compiler), '--language', language, '--sdk', str(root / 'user/sdk'),
            '--intrinsics', intrinsics, '--provider', str(provider),
            '--math-include', str(math_include)]
    if language=='cpp' and options.cxx_configuration:
        result+=['--cxx-library','--','-isystem',str(cxx_configuration/'build/include'),
                 '-isystem',str(cxx_configuration/'build/include/x86_64-unknown-myos'),
                 '-isystem',str(cxx_configuration/'gcc-15.2.0/libstdc++-v3/libsupc++')]
    else:result+=['--']
    return result
# Test the boundary itself before trusting configure results. This is link
# evidence only: never execute MyOS syscall instructions on the Linux host.
boundary_source = '''#include <pthread.h>
#include <time.h>
#include <math.h>
#include <stdlib.h>
#include <unistd.h>
#if defined(__linux__) || defined(__GLIBC__)
#error Host OS ABI leaked into MyOS probe
#endif
_Static_assert(sizeof(pthread_mutex_t) == 80, "Cobalt mutex ABI");
_Static_assert(sizeof(pthread_cond_t) == 96, "Cobalt cond ABI");
int main(void) { struct timespec t; double (*volatile f)(double)=sqrt;
return clock_gettime(CLOCK_MONOTONIC, &t) + (int)f(4.0) + getgid() + getegid()
       + !!getenv("MYOS_BOUNDARY_PROBE"); }
'''
boundary = out / 'runtime-boundary'
boundary_result = subprocess.run(command('c') + ['-x', 'c', '-', '-o', str(boundary)],
                                 input=boundary_source, text=True, capture_output=True)
assert boundary_result.returncode == 0, boundary_result.stderr[-1200:]
dynamic = subprocess.check_output(['readelf', '-dW', str(boundary)], text=True)
assert '(NEEDED)' not in dynamic, dynamic
assert not subprocess.check_output(['nm', '-u', str(boundary)], text=True).strip()
rejected = subprocess.run(command('c') + ['-lm', '-x', 'c', '-', '-o', str(out / 'forbidden')],
                          input='int main(void) { return 0; }', text=True, capture_output=True)
assert rejected.returncode and 'rejects unverified' in rejected.stderr
if options.cxx_configuration:
    text='''#include <vector>
#include <unordered_map>
int main() { std::vector<unsigned long> v(5,7);
std::unordered_map<unsigned long,unsigned long> m; m.emplace(3,v[2]);
m.reserve(32); return m.at(3)==7?0:1; }
'''
    result=subprocess.run(command('cpp')+['-std=c++17','-x','c++','-','-o',str(out/'cxx-boundary')],
                          input=text,text=True,capture_output=True)
    assert result.returncode==0,result.stderr[-2000:]
    assert '(NEEDED)' not in subprocess.check_output(['readelf','-dW',str(out/'cxx-boundary')],text=True)
    assert not subprocess.check_output(['nm','-u',str(out/'cxx-boundary')],text=True).strip()
cross = out / 'myos.ini'
cross.write_text('[binaries]\nc = ' + repr(command('c')) + '\ncpp = ' + repr(command('cpp')) +
                 '\nar = \'ar\'\nstrip = \'strip\'\npkg-config = \'false\'\n' +
                 '[host_machine]\nsystem = \'myos\'\ncpu_family = \'x86_64\'\n' +
                 'cpu = \'x86_64\'\nendian = \'little\'\n' +
                 '[properties]\nneeds_exe_wrapper = true\n' +
                 '[built-in options]\nc_std = \'c11\'\ncpp_std = \'c++17\'\n')
env = os.environ.copy()
env['PATH'] = str(base / 'venv/bin') + ':' + str(base / 'toolroot/usr/bin') + ':' + env['PATH']
env['BISON_PKGDATADIR'] = str(base / 'toolroot/usr/share/bison')
env['M4'] = str(base / 'toolroot/usr/bin/m4')
args = ['meson', 'setup', str(out / 'build'), str(source), '--cross-file', str(cross),
        '--buildtype=release', '--wrap-mode=nofallback', '-Dgallium-drivers=softpipe',
        '-Dvulkan-drivers=[]', '-Dplatforms=[]', '-Dglx=disabled', '-Degl=disabled',
        '-Dosmesa=true', '-Dgles1=disabled', '-Dgles2=enabled', '-Dllvm=disabled',
        '-Dgbm=disabled', '-Dgallium-va=disabled', '-Dgallium-vdpau=disabled',
        '-Dvideo-codecs=[]', '-Dglvnd=disabled', '-Dbuild-tests=false',
        '-Dshader-cache=disabled', '-Dzlib=disabled', '-Dzstd=disabled', '-Dexpat=disabled',
        '-Dxmlconfig=disabled', '-Dlibunwind=disabled', '-Dvalgrind=disabled',
        '-Dlmsensors=disabled', '-Dcpp_rtti=false', '-Dcpp_eh=none']
log = out / 'configure.log'
with log.open('w') as stream:
    result = subprocess.run(args, env=env, stdout=stream, stderr=subprocess.STDOUT)
report = dict(output=str(out.relative_to(root)), configure_exit=result.returncode,
              boundary_checks_passed=True,
              math_sdk_integrated=True,
              cxx_container_sdk_integrated=bool(options.cxx_configuration),
              host_libraries_allowed=False, native_execution=False,
              native_backend_selected=False)
exit_code = result.returncode
if options.compile and not exit_code:
    compile_log = out / 'compile.log'
    with compile_log.open('w') as stream:
        compiled = subprocess.run(['ninja', '-C', str(out / 'build'), '-j2', '-k0' if options.keep_going else '-k8'],
                                  env=env, stdout=stream, stderr=subprocess.STDOUT)
    report['compile_exit'] = compiled.returncode
    exit_code = compiled.returncode
    errors = [line for line in compile_log.read_text().splitlines()
              if 'error:' in line or line.startswith('FAILED:')]
    report['compile_failure_limit'] = 0 if options.keep_going else 8
    report['failed_units'] = sum(line.startswith('FAILED:') for line in errors)
    report['error_count'] = sum('error:' in line for line in errors)
    print('\n'.join(errors[:30]), flush=True)
(out / 'report.json').write_text(json.dumps(report, indent=2) + '\n')
print(json.dumps(report), flush=True)
print('\n'.join(log.read_text().splitlines()[-30:]), flush=True)
raise SystemExit(exit_code)
