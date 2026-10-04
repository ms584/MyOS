"""Incremental compiler-only renderer closure; NEVER execute its probe runtime."""
import argparse
import json
import os
from pathlib import Path
import runpy
import shlex
import subprocess
from build_cxx_containers_sdk import build

root=Path(__file__).resolve().parents[1]
parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--renderer-probe',type=Path,required=True)
parser.add_argument('--configuration',type=Path,required=True)
args=parser.parse_args()
base=root/'build/renderer-evaluation'
out=args.renderer_probe.resolve();cfg=args.configuration.resolve()
assert out.is_relative_to(base.resolve()) and out.name.startswith('target-config-')
assert cfg.is_relative_to((root/'build/cxx-target-evaluation').resolve())
original=json.loads((out/'report.json').read_text())
assert original['configure_exit']==0 and original['boundary_checks_passed']
assert original['cxx_container_sdk_integrated']
assert original['host_libraries_allowed'] is False
assert str(cfg) in (out/'myos.ini').read_text()
provider=out/'libmyos-runtime-probe.a'
math=runpy.run_path(str(root/'tools/build_math_sdk.py'))
subprocess.run(['ar','rcs',str(provider),*map(str,math['objects'].values())],check=True)
containers=build(cfg,provider)
subprocess.run(['ar','rcs',str(provider),*map(str,containers['cxx_objects'])],check=True)
env=os.environ.copy()
env['PATH']=str(base/'venv/bin')+':'+str(base/'toolroot/usr/bin')+':'+env['PATH']
env['BISON_PKGDATADIR']=str(base/'toolroot/usr/share/bison')
env['M4']=str(base/'toolroot/usr/bin/m4')
with (out/'closure-link.log').open('w') as log:
    result=subprocess.run([str(base/'venv/bin/ninja'),'-C',str(out/'build'),'-j2','-k0'],
                          env=env,stdout=log,stderr=subprocess.STDOUT)
    if result.returncode==0:
        # The appended provider and Python wrapper are not Ninja dependencies.
        # Replay only Ninja's actual final link command, never all compilation.
        target='src/gallium/targets/osmesa/libOSMesa.so.8.0.0'
        commands=subprocess.check_output([str(base/'venv/bin/ninja'),'-C',str(out/'build'),
                                          '-t','commands',target],env=env,text=True)
        command=shlex.split(commands.splitlines()[-1])
        assert command[:2]==['python3',str(root/'tools/renderer_target_compiler.py')]
        assert '-shared' in command and command[command.index('-o')+1]==target
        print('Explicit final link:',shlex.join(command),file=log,flush=True)
        result=subprocess.run(command,cwd=out/'build',env=env,
                              stdout=log,stderr=subprocess.STDOUT)
library=out/'build/src/gallium/targets/osmesa/libOSMesa.so.8.0.0'
report=dict(ninja_exit=result.returncode,native_execution=False,
            native_backend_selected=False,host_libraries_allowed=False,
            output=str(out.relative_to(root)),static_renderer_link_passed=False,
            configure_only_runtime=True)
if result.returncode==0:
    dynamic=subprocess.check_output(['readelf','-dW',str(library)],text=True)
    undefined=subprocess.check_output(['nm','-u',str(library)],text=True)
    needed=[line.strip() for line in dynamic.splitlines() if '(NEEDED)' in line]
    strong=[line.strip() for line in undefined.splitlines() if line.split()[0]=='U']
    weak=[line.split()[-1] for line in undefined.splitlines() if line.split()[0] in ('w','v')]
    segments=subprocess.check_output(['readelf','-lW',str(library)],text=True)
    report.update(static_renderer_link_passed=not strong and not needed,
                  strong_undefined_symbols=strong,needed_libraries=needed,
                  renderer_bytes=library.stat().st_size,
                  weak_undefined_symbols=weak,tls_segment_present=any(
                      line.strip().startswith('TLS ') for line in segments.splitlines()))
(out/'incremental-report.json').write_text(json.dumps(report,indent=2)+'\n')
print(json.dumps(report,indent=2),flush=True)
raise SystemExit(result.returncode or (0 if report['static_renderer_link_passed'] else 1))
