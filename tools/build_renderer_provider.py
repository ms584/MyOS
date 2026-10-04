"""Package genuine EGL/GLES provider from a verified native fixture closure.

No fixture entry point, host loader/libraries or claim of provider execution.
The evidence log is parsed as data; only validated build paths become linker args.
"""
import argparse
import hashlib
import json
from pathlib import Path
import re
import shlex
import struct
import subprocess
import tempfile

root=Path(__file__).resolve().parents[1]
parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--fixture',type=Path,required=True)
parser.add_argument('--evidence-log',type=Path,required=True)
args=parser.parse_args()
fixture=args.fixture.resolve();evidence=args.evidence_log.resolve()
assert fixture.is_relative_to((root/'build/native-renderer-evaluation').resolve())
assert evidence.is_relative_to((root/'build/renderer-evaluation').resolve())
log=evidence.read_text()
assert 'PASS: actual native Mesa' in log and f'NOT Cobalt. {fixture}\n' in log
commands=[shlex.split(line[2:]) for line in log.splitlines() if line.startswith('$ ld -shared ')]
command=next(cmd for cmd in commands if str(fixture/'renderer.so') in cmd)
assert command[:2]==['ld','-shared'] and '--start-group' in command and '--end-group' in command
assert not any(item.startswith('--wrap=') for item in command)
supports={fixture/name for name in ('finalize.o','environment.o','process-exit.o')}
inputs=[]
for item in command:
    if not item.endswith(('.o','.a')):continue
    path=Path(item).resolve()
    assert path.is_relative_to((root/'build').resolve()) and path.is_file()
    if path.suffix=='.a' or path in supports or path.name=='osmesa_target.c.o':inputs.append(path)
assert supports.issubset(inputs) and any(path.name=='libmyos-egl-core.a' for path in inputs)
base=root/'build/renderer-provider-evaluation';base.mkdir(parents=True,exist_ok=True)
out=Path(tempfile.mkdtemp(prefix='provider-',dir=base));library=out/'provider.so'
objects=[path for path in inputs if path.suffix=='.o']
archives=[path for path in inputs if path.suffix=='.a']
link=['ld','-shared','--gc-sections','--hash-style=both','-Bsymbolic',
      '-z','now','-z','relro','-z','noexecstack','-u','SbGetEglInterface',
      '-u','SbGetGlesInterface','-o',str(library),*map(str,objects),
      '--start-group',*map(str,archives),'--end-group']
with (out/'link.log').open('w') as stream:
    stream.write('$ '+shlex.join(link)+'\n');stream.flush()
    subprocess.run(link,stdout=stream,stderr=subprocess.STDOUT,check=True)
dynamic=subprocess.check_output(['readelf','-dW',str(library)],text=True)
symbols=subprocess.check_output(['readelf','--dyn-syms','-W',str(library)],text=True)
relocations=subprocess.check_output(['readelf','-rW',str(library)],text=True)
undefined=subprocess.check_output(['nm','-u',str(library)],text=True)
for name,value in [('dynamic.txt',dynamic),('symbols.txt',symbols),
                   ('relocations.txt',relocations),('undefined.txt',undefined)]:
    (out/name).write_text(value)
assert '(NEEDED)' not in dynamic and '(TEXTREL)' not in dynamic
blob=library.read_bytes()
assert blob[:7]==b'\x7fELF\x02\x01\x01' and struct.unpack_from('<HH',blob,16)==(3,62)
assert struct.unpack_from('<Q',blob,24)[0]==0
phoff=struct.unpack_from('<Q',blob,32)[0]
phsize,phcount=struct.unpack_from('<HH',blob,54);assert phsize==56
segments=[struct.unpack_from('<IIQQQQQQ',blob,phoff+i*56) for i in range(phcount)]
loads=[part for part in segments if part[0]==1]
assert loads and not any(part[0]==3 for part in segments)
assert all(part[1] in (4,5,6) and part[5]<=part[6] and part[2]+part[5]<=len(blob)
           for part in loads)
exports={}
for line in symbols.splitlines():
    parts=line.split()
    if len(parts)!=8 or parts[7] not in ('SbGetEglInterface','SbGetGlesInterface'):continue
    assert parts[3:6]==['FUNC','GLOBAL','DEFAULT'] and parts[6].isdigit()
    address,size=int(parts[1],16),int(parts[2]);assert size>0
    assert any(part[1]==5 and address>=part[3] and
               address+size<=part[3]+part[5] for part in loads)
    exports[parts[7]]=dict(offset=address,size=size)
assert len(exports)==2
allowlist=set(re.findall(r'\bX\((\w+),',(root/'build/platform-runtime-exports.h').read_text()))
strong=[line.split()[-1] for line in undefined.splitlines() if line.split()[0]=='U']
assert set(strong).issubset(allowlist),sorted(set(strong)-allowlist)
relocation_types=sorted(set(re.findall(r'\bR_X86_64_\w+',relocations)))
supported={'R_X86_64_RELATIVE','R_X86_64_64','R_X86_64_GLOB_DAT','R_X86_64_JUMP_SLOT',
           'R_X86_64_DTPMOD64','R_X86_64_DTPOFF64','R_X86_64_TPOFF64','R_X86_64_TLSDESC'}
assert set(relocation_types).issubset(supported),relocation_types
digest=lambda path:hashlib.sha256(path.read_bytes()).hexdigest()
tls=[dict(offset=part[3],filesz=part[5],memsz=part[6],align=part[7])
     for part in segments if part[0]==7]
report=dict(output=str(out.relative_to(root)),provider_sha256=digest(library),
    bytes=len(blob),load_memory_bytes=sum(part[6] for part in loads),
    entry=0,exports=exports,tls=tls,relocations=relocation_types,
    strong_imports=strong,missing_platform_imports=[],host_libraries_allowed=False,
    input_fixture=str(fixture),fixture_sha256=digest(fixture/'renderer.so'),
    evidence_log=str(evidence),evidence_sha256=digest(evidence),
    inputs=[dict(path=str(path),sha256=digest(path)) for path in inputs],
    native_provider_executed=False,cobalt_acceptance_passed=False)
(out/'report.json').write_text(json.dumps(report,indent=2)+'\n')
print(json.dumps({key:report[key] for key in ('output','bytes','load_memory_bytes','exports',
    'tls','relocations','missing_platform_imports','native_provider_executed')}),flush=True)
