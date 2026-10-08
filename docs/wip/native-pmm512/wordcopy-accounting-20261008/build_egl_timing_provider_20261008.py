"""Diagnostic provider, same Mesa archive except one instrumented EGL driver."""
from pathlib import Path
import hashlib,json,re,shlex,shutil,struct,subprocess,tempfile
root=Path('/home/xytron404/cobalt-closed-imports-xnVZeH')
stage=Path(__file__).resolve().parent
base=root/'build/renderer-provider-evaluation/provider-84sokh6g'
digest=lambda p:hashlib.sha256(Path(p).read_bytes()).hexdigest()
report=json.loads((base/'report.json').read_text())
assert digest(base/'provider.so')==report['provider_sha256']
for item in report['inputs']+report['linked_objects']:assert digest(item['path'])==item['sha256'],item['path']
original=next(Path(item['path']) for item in report['inputs'] if Path(item['path']).name=='libmyos-egl-core.a')
egl=json.loads((original.parent/'report.json').read_text())
probe=Path(egl['renderer_probe']);probe=probe if probe.is_absolute() else root/probe
commands=json.loads((probe/'build/compile_commands.json').read_text())
cwd=next(item['directory'] for item in commands if item['file'].endswith('/osmesa.c'))
compile_lines=[shlex.split(line[2:]) for line in (original.parent/'compile.log').read_text().splitlines() if line.startswith('$ ')]
compile_cmd=next(cmd for cmd in compile_lines if '-c' in cmd and Path(cmd[cmd.index('-c')+1]).name=='platform-egl-driver.c')
assert '-o' in compile_cmd and not any(arg.startswith('--wrap') for arg in compile_cmd)
out=Path(tempfile.mkdtemp(prefix='egl-timing-',dir=root/'build/renderer-provider-evaluation'))
obj=out/'platform-egl-driver.o';source=stage/'platform-egl-timing-20261008.c'
compile_cmd[compile_cmd.index('-c')+1]=str(source);compile_cmd[compile_cmd.index('-o')+1]=str(obj)
with (out/'compile.log').open('w') as log:
 print('$ '+shlex.join(compile_cmd),file=log,flush=True)
 subprocess.run(compile_cmd,cwd=cwd,stdout=log,stderr=subprocess.STDOUT,check=True)
archive=out/original.name;shutil.copyfile(original,archive)
members=subprocess.check_output(['ar','t',str(original)],text=True).splitlines()
assert members.count(obj.name)==1
subprocess.run(['ar','rcs',str(archive),str(obj)],check=True)
assert subprocess.check_output(['ar','t',str(archive)],text=True).splitlines()==members
for member in members:
 if member!=obj.name:assert subprocess.check_output(['ar','p',str(original),member])==subprocess.check_output(['ar','p',str(archive),member]),member
link=shlex.split((base/'link.log').read_text().splitlines()[0][2:])
assert link[:2]==['ld','-shared'] and link.count(str(original))==1
assert not any(arg.startswith('--wrap') for arg in link)
link[link.index(str(original))]=str(archive);link[link.index('-o')+1]=str(out/'provider.so')
with (out/'link.log').open('w') as log:
 print('$ '+shlex.join(link),file=log,flush=True);subprocess.run(link,stdout=log,stderr=subprocess.STDOUT,check=True)
library=out/'provider.so';blob=library.read_bytes()
dynamic=subprocess.check_output(['readelf','-dW',str(library)],text=True)
assert '(NEEDED)' not in dynamic and '(TEXTREL)' not in dynamic
strong=sorted(line.split()[-1] for line in subprocess.check_output(['nm','-u',str(library)],text=True).splitlines() if line.split()[0]=='U')
assert set(strong)-set(report['strong_imports'])<= {'SbLogRaw'}
allowed=set(re.findall(r'\bX\((\w+),',(root/'build/platform-runtime-exports.h').read_text()))
assert set(strong)<=allowed
phoff=struct.unpack_from('<Q',blob,32)[0];phsize,phcount=struct.unpack_from('<HH',blob,54);assert phsize==56
headers=[struct.unpack_from('<IIQQQQQQ',blob,phoff+i*phsize) for i in range(phcount)]
loads=[header for header in headers if header[0]==1]
assert loads and not any(header[0]==3 for header in headers)
assert all(h[1] in (4,5,6) and h[5]<=h[6] and h[2]+h[5]<=len(blob) for h in loads)
exports={}
for line in subprocess.check_output(['nm','-D','-S','--defined-only',str(library)],text=True).splitlines():
 fields=line.split()
 if len(fields)==4 and fields[-1] in report['exports']:exports[fields[-1]]=dict(offset=int(fields[0],16),size=int(fields[1],16))
assert exports.keys()==report['exports'].keys()
for export in exports.values():assert any(h[1]==5 and h[3]<=export['offset'] and export['offset']+export['size']<=h[3]+h[5] for h in loads)
relocations=sorted(set(re.findall(r'\bR_X86_64_\w+',subprocess.check_output(['readelf','-rW',str(library)],text=True))))
assert relocations==sorted(report['relocations'])
new=dict(report,output=str(out.relative_to(root)),provider_sha256=digest(library),bytes=len(blob),exports=exports,strong_imports=strong,
 load_memory_bytes=sum(h[6] for h in loads),tls=[dict(offset=h[3],filesz=h[5],memsz=h[6],align=h[7]) for h in headers if h[0]==7],
 relocations=relocations,entry=struct.unpack_from('<Q',blob,24)[0],native_provider_executed=False,
 experiment='DIAGNOSTIC raw EGL TSC split; no performance/acceptance claim',baseline_provider_sha256=report['provider_sha256'],
 changed_archive_member=obj.name,experimental_source_sha256=digest(source),experimental_compile=compile_cmd,experimental_link=link)
new['inputs']=[dict(path=str(archive),sha256=digest(archive)) if item['path']==str(original) else item for item in report['inputs']]
(out/'report.json').write_text(json.dumps(new,indent=2)+'\n')
assert digest(base/'provider.so')==report['provider_sha256']
assert digest(original)==next(item['sha256'] for item in report['inputs'] if item['path']==str(original))
(root/'egl-timing-provider-path.txt').write_text(str(out)+'\n')
print('DIAGNOSTIC_PROVIDER',out,digest(library),flush=True)
