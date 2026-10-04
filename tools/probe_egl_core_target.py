"""Compile genuine Mesa EGL core with its existing MyOS target profile.

Compile/dependency evidence only; --driver includes the real MyOS driver.
No fake provider, host execution or native EGL pass is inferred from compilation.
Isolated outputs do not modify production boot images.
"""
import argparse
import hashlib
import json
from pathlib import Path
import shlex
import shutil
import subprocess
import tempfile

root=Path(__file__).resolve().parents[1]
parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--renderer-probe',type=Path,required=True)
parser.add_argument('--driver',action='store_true',help='Compile the actual MyOS pbuffer/context driver')
args=parser.parse_args()
probe=args.renderer_probe.resolve()
assert probe.is_relative_to((root/'build/renderer-evaluation').resolve())
archive=root/'build/renderer-evaluation/mesa-25.0.7.tar.xz'
with archive.open('rb') as stream:
    assert hashlib.file_digest(stream,'sha256').hexdigest()==(
        '592272df3cf01e85e7db300c449df5061092574d099da275d19e97ef0510f8a6')
commands=json.loads((probe/'build/compile_commands.json').read_text())
item=next(value for value in commands if value['file'].endswith('/osmesa.c'))
original=shlex.split(item['command'])
assert str(root/'tools/renderer_target_compiler.py') in original
assert '-c' in original and '-o' in original
# Reuse actual cross-profile/configuration flags, not Linux EGL/pkg-config.
flags=[];index=0
while index<len(original):
    value=original[index]
    if value in ('-o','-MF','-MQ','-MT'):
        index+=2;continue
    if value in ('-c','-MD','-MMD') or value==item['file']:
        index+=1;continue
    flags.append(value);index+=1
source=probe/'mesa-25.0.7'
base=root/'build/egl-core-evaluation';base.mkdir(parents=True,exist_ok=True)
out=Path(tempfile.mkdtemp(prefix='core-',dir=base))
egl_source=out/'egl-main'
shutil.copytree(source/'src/egl/main',egl_source)
subprocess.run(['patch','--batch','--fuzz=0','-p1','-i',str(root/'tools/mesa_egl_myos_platform.patch')],
               cwd=egl_source,check=True)
sources=[egl_source/(name+'.c') for name in (
    'eglapi','eglarray','eglconfigdebug','eglconfig','eglcontext','eglcurrent',
    'egldevice','egldisplay','eglglobals','eglimage','egllog','eglsurface','eglsync')]
sources.append(source/'src/util/rwlock.c')
if args.driver:sources.extend([root/'user/platform-egl-driver.c',source/'src/util/os_file.c'])
objects=[];failed=[]
with (out/'compile.log').open('w') as log:
    for path in sources:
        obj=out/(path.stem+'.o')
        command=[*flags,'-I'+str(egl_source),'-I'+str(root/'include'),'-DEGL_NO_X11=1',
                 '-D_EGL_NATIVE_PLATFORM=_EGL_PLATFORM_MYOS',
                 '-c',str(path),'-o',str(obj)]
        log.write('$ '+shlex.join(command)+'\n');log.flush()
        result=subprocess.run(command,cwd=item['directory'],stdout=log,stderr=subprocess.STDOUT)
        if result.returncode:failed.append(path.name)
        else:objects.append(obj)
    if objects:
        subprocess.run(['ar','rcs',str(out/'libmyos-egl-core.a'),*map(str,objects)],check=True)
        subprocess.run(['nm','-u',str(out/'libmyos-egl-core.a')],stdout=log,check=True)
undefined=[]
if not failed:
    combined=out/'egl-core.o'
    subprocess.run(['ld','-r','-o',str(combined),*map(str,objects)],check=True)
    symbols=subprocess.check_output(['nm','-u',str(combined)],text=True)
    (out/'undefined.txt').write_text(symbols)
    undefined=[line.split()[-1] for line in symbols.splitlines()]
report=dict(output=str(out.relative_to(root)),compiled_units=len(objects),
            failed_units=failed,total_units=len(sources),host_libraries_allowed=False,
            renderer_probe=str(probe),undefined_symbols=undefined,native_egl_passed=False,
            driver_compiled=bool(args.driver and not failed),driver_implemented=bool(args.driver))
(out/'report.json').write_text(json.dumps(report,indent=2)+'\n')
print(json.dumps(report),flush=True)
print('\n'.join(line for line in (out/'compile.log').read_text().splitlines()
                if 'error:' in line)[:4000],flush=True)
raise SystemExit(bool(failed))
