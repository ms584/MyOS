"""Probe real libstdc++ cross configuration without host libraries/native claims.

Generated sources/logs are isolated. Does not alter SDK, kernel or boot images.
Target recognition and real cross capability probes are patched. No Linux
capabilities or /dev/random are assumed; unsupported target facilities fail.
"""
from pathlib import Path
import argparse
import hashlib
import json
import os
import shlex
import subprocess
import tempfile

root=Path(__file__).resolve().parents[1]
parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--renderer-probe',type=Path,required=True,
                    help='Existing checked MyOS provider/intrinsic/math boundary')
args=parser.parse_args()
previous=args.renderer_probe.resolve()
assert previous.is_relative_to((root/'build/renderer-evaluation').resolve())
provider=previous/'libmyos-runtime-probe.a'
assert provider.is_file() and (previous/'math-include/math.h').is_file()
base=root/'build/cxx-target-evaluation'
archive=base/'gcc-15.2.0.tar.xz'
with archive.open('rb') as stream:
    assert hashlib.file_digest(stream,'sha512').hexdigest()==(
        '89047a2e07bd9da265b507b516ed3635adb17491c7f4f67cf090f0bd5b3fc7f2ee6e4cc4008beef7ca884b6b71dffe2bb652b21f01a702e17b468cca2d10b2de')
assert subprocess.check_output(['gcc','-dumpfullversion'],text=True).strip()=='15.2.0'
out=Path(tempfile.mkdtemp(prefix='configure-',dir=base))
members=['libstdc++-v3','libgcc/gthr.h','libgcc/gthr-posix.h','libgcc/gthr-single.h',
         'gcc/BASE-VER','gcc/DATESTAMP',
         'COPYING3','COPYING.RUNTIME','config','config.guess','config.sub','config-ml.in',
         'install-sh','ltmain.sh','missing','mkinstalldirs','move-if-change','compile','depcomp']
subprocess.run(['tar','-xJf',str(archive),'-C',str(out),
                *['gcc-15.2.0/'+name for name in members]],check=True)
source=out/'gcc-15.2.0'
for check in (True,False):
    subprocess.run(['git','apply',*(['--check'] if check else []),
                    '--directory='+str(source.relative_to(root)),str(root/'tools/gcc_myos_target.patch')],
                   cwd=root,check=True)
target='x86_64-unknown-myos'
assert subprocess.check_output(['sh',str(source/'config.sub'),target],text=True).strip()==target
intrinsics=subprocess.check_output(['gcc','-print-file-name=include'],text=True).strip()
def command(language):
    return ['python3',str(root/'tools/renderer_target_compiler.py'),'--language',language,
            '--sdk',str(root/'user/sdk'),'--intrinsics',intrinsics,'--provider',str(provider),
            '--math-include',str(previous/'math-include'),
            *(['--cxx-library'] if language=='cpp' else []),'--']
env=os.environ.copy()
env.update(CC=shlex.join(command('c')),CXX=shlex.join(command('cpp')),
           CFLAGS='-O2',CXXFLAGS='-O2 -fno-exceptions -fno-rtti')
build=out/'build';build.mkdir()
options=['sh',str(source/'libstdc++-v3/configure'),
         '--build=x86_64-pc-linux-gnu','--host='+target,'--target='+target,
         '--disable-multilib','--disable-shared','--enable-static',
         '--disable-nls','--disable-libstdcxx-pch','--enable-clocale=generic']
log=out/'configure.log'
with log.open('w') as output:
    result=subprocess.run(options,cwd=build,env=env,stdout=output,stderr=subprocess.STDOUT)
lines=log.read_text().splitlines()
header=build/'include'/target/'bits/c++config.h'
complete=result.returncode==0 and header.is_file()
errors=[line for line in lines if 'error:' in line or 'make: ***' in line]
if not header.is_file():
    errors.append('Generated target bits/c++config.h is missing')
report={'output':str(out.relative_to(root)),'target':target,'configure_exit':result.returncode,
        'source_version':'15.2.0','source_checksum_verified':True,
        'host_libraries_allowed':False,'source_configuration_complete':complete,
        'native_execution':False,'renderer_linked':False,
        'errors':errors}
(out/'report.json').write_text(json.dumps(report,indent=2)+'\n')
print(json.dumps(report),flush=True)
print('\n'.join(lines[-25:]),flush=True)
raise SystemExit(result.returncode or (0 if complete else 1))
