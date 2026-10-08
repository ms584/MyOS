"""Private provider experiment: replace ONE archive member, pin all other inputs.
No native execution/performance claim. Never overwrites baseline provider/archive.
"""
from pathlib import Path
import hashlib,json,re,shlex,shutil,struct,subprocess,tempfile
root=Path('/home/xytron404/cobalt-closed-imports-xnVZeH')
stage=Path('/mnt/c/Users/Xytron/Documents/Codex/2026-09-30/myos-checkpoint-9f58024-home-xytron404-myos-2/work')
base=root/'build/renderer-provider-evaluation/provider-84sokh6g'
report=json.loads((base/'report.json').read_text())
digest=lambda p:hashlib.sha256(Path(p).read_bytes()).hexdigest()
assert digest(base/'provider.so')==report['provider_sha256']
for item in report['inputs']:assert digest(item['path'])==item['sha256'],item['path']
for item in report['linked_objects']:assert digest(item['path'])==item['sha256'],item['path']
out=Path(tempfile.mkdtemp(prefix='wordcopy-',dir=root/'build/renderer-provider-evaluation'))
source=stage/'platform-sdk-string-wordcopy-20261008.c'
include=subprocess.check_output(['gcc','-print-file-name=include'],text=True).strip()
flags=['gcc','-m64','-std=c11','-Os','-fPIC','-ffreestanding','-fno-builtin','-mno-red-zone',
 '-fno-stack-protector','-nostdinc','-isystem',include,'-I/home/xytron404/myos/user/sdk']
# Match baseline primitives compilation; only the C implementation differs.
subprocess.run([*flags,'-c',str(source),'-o',str(out/'primitives.o')],check=True)
# Rename exported definitions in a separate sanitizer object, so host libc and
# sanitizer internals cannot accidentally call or interpose the trial routines.
host=out/'host-primitives.o'
subprocess.run([*flags,'-fsanitize=address,undefined','-g','-c',str(source),'-o',str(host)],check=True)
defined=subprocess.check_output(['nm','--defined-only',str(host)],text=True)
names=[line.split()[-1] for line in defined.splitlines() if len(line.split())==3 and line.split()[1]=='T']
subprocess.run(['objcopy',*[arg for name in names for arg in ('--redefine-sym',name+'=trial_'+name)],str(host)],check=True)
subprocess.run(['gcc','-std=c11','-O2','-Wall','-Wextra','-Werror','-fsanitize=address,undefined',
 str(stage/'sdk-wordcopy-test-20261008.c'),str(host),'-o',str(out/'host-test')],check=True)
subprocess.run([str(out/'host-test')],check=True)
original=Path('/home/xytron404/myos/build/cxx-container-evaluation/sdk-fduxdr2t/libmyos-cxx-containers.a')
archive=out/original.name;shutil.copyfile(original,archive)
members=subprocess.check_output(['ar','t',str(original)],text=True).splitlines()
assert members.count('primitives.o')==1
subprocess.run(['ar','rcs',str(archive),str(out/'primitives.o')],check=True)
assert subprocess.check_output(['ar','t',str(archive)],text=True).splitlines()==members
for member in members:
 if member!='primitives.o':
  assert subprocess.check_output(['ar','p',str(original),member])==subprocess.check_output(['ar','p',str(archive),member]),member
command=shlex.split((base/'link.log').read_text().splitlines()[0][2:])
assert command[:2]==['ld','-shared'] and not any(arg.startswith('--wrap') for arg in command)
assert command.count(str(original))==1
command[command.index(str(original))]=str(archive)
command[command.index('-o')+1]=str(out/'provider.so')
with (out/'link.log').open('w') as stream:
 print('$ '+shlex.join(command),file=stream,flush=True)
 subprocess.run(command,stdout=stream,stderr=subprocess.STDOUT,check=True)
library=out/'provider.so';blob=library.read_bytes()
dynamic=subprocess.check_output(['readelf','-dW',str(library)],text=True)
assert '(NEEDED)' not in dynamic and '(TEXTREL)' not in dynamic
undefined=subprocess.check_output(['nm','-u',str(library)],text=True)
strong=sorted(line.split()[-1] for line in undefined.splitlines() if line.split()[0]=='U')
assert strong==sorted(report['strong_imports']),'import contract changed'
exports={}
for line in subprocess.check_output(['nm','-D','-S','--defined-only',str(library)],text=True).splitlines():
 fields=line.split()
 if len(fields)==4 and fields[-1] in report['exports']:
  exports[fields[-1]]=dict(offset=int(fields[0],16),size=int(fields[1],16))
assert exports.keys()==report['exports'].keys()
phoff=struct.unpack_from('<Q',blob,32)[0];phsize,phcount=struct.unpack_from('<HH',blob,54)
headers=[struct.unpack_from('<IIQQQQQQ',blob,phoff+i*phsize) for i in range(phcount)]
loads=[header for header in headers if header[0]==1]
assert loads and not any(header[0]==3 for header in headers)
assert all(header[1] in (4,5,6) and header[5]<=header[6] and header[2]+header[5]<=len(blob) for header in loads)
for export in exports.values():
 assert any(header[1]==5 and header[3]<=export['offset'] and export['offset']+export['size']<=header[3]+header[5] for header in loads)
relocations=sorted(set(re.findall(r'\bR_X86_64_\w+',subprocess.check_output(['readelf','-rW',str(library)],text=True))))
assert relocations==sorted(report['relocations']),'relocation contract changed'
new=dict(report,output=str(out.relative_to(root)),provider_sha256=digest(library),bytes=len(blob),
 exports=exports,load_memory_bytes=sum(header[6] for header in headers if header[0]==1),
 tls=[dict(offset=header[3],filesz=header[5],memsz=header[6],align=header[7]) for header in headers if header[0]==7],
 relocations=relocations,
 entry=struct.unpack_from('<Q',blob,24)[0],native_provider_executed=False,
 experiment='64-bit bounded integer copy; host correctness passed, native performance pending',
 baseline_provider_sha256=report['provider_sha256'],changed_archive_member='primitives.o',
 experimental_source_sha256=digest(source),experimental_archive_sha256=digest(archive),
 experimental_link=command)
new['inputs']=[dict(path=item['path'],sha256=item['sha256']) if item['path']!=str(original)
 else dict(path=str(archive),sha256=digest(archive)) for item in report['inputs']]
(out/'report.json').write_text(json.dumps(new,indent=2)+'\n')
assert digest(base/'provider.so')==report['provider_sha256'] and digest(original)==next(item['sha256'] for item in report['inputs'] if item['path']==str(original))
(root/'wordcopy-provider-path.txt').write_text(str(out)+'\n')
print('EXPERIMENTAL_PROVIDER',out,'sha256',digest(library),'host correctness only; native results pending',flush=True)
