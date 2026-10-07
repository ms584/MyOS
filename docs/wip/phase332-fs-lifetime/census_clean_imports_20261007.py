"""Measure bindings only, NEVER browser or semantic acceptance."""
import hashlib,json,re,subprocess,sys
from pathlib import Path
main=Path('/home/xytron404/myos')
audit=json.loads((main/'docs/PHASE33_COBALT_AUDIT.json').read_text())
core=main/'build/engine-evaluation/libcobalt.so'
assert hashlib.sha256(core.read_bytes()).hexdigest()==audit['library_sha256']
# Audit names include one weak symbol. The242 denominator is GLOBAL UND,
# not all names or function stubs. nm U (not w) is the actual pinned set.
required={line.split()[-1] for line in subprocess.check_output(
 ['nm','-D','--undefined-only',str(core)],text=True).splitlines()
 if len(line.split())>=2 and line.split()[-2]=='U'}
assert required<=set(audit['undefined_symbols'])
assert len(required)==242
for root in (tuple(Path(value) for value in sys.argv[1:]) if len(sys.argv)>1 else (main,Path('/home/xytron404/cd-QJYMwG'))):
 provider=main/'build/renderer-provider-evaluation/provider-84sokh6g/provider.so'
 report=json.loads((provider.parent/'report.json').read_text())
 assert hashlib.sha256(provider.read_bytes()).hexdigest()==report['provider_sha256']
 def strong(path,dynamic=False):
  args=['nm','-g','--defined-only']+(['-D'] if dynamic else [])+[str(path)]
  result=set()
  for line in subprocess.check_output(args,text=True).splitlines():
   words=line.split()
   if len(words)==3 and words[1] in ('T','D','B','R','A','S','G'):result.add(words[2])
  return result
 names=set(re.findall(r'\bX\(([^,]+),\s*PLATFORM_', (root/'build/platform-runtime-exports.h').read_text()))
 definitions=strong(root/'build/platform-runtime.elf')
 assert not any(name.startswith('MyOSMissingImport') for name in definitions)
 assert names<=definitions,sorted(names-definitions)
 paired=(names&definitions)|strong(provider,True)
 print(root,'registered',len(required&names),'strong paired',len(required&paired),
       'required',len(required),'missing',len(required-paired),flush=True)
 print('Missing:',', '.join(sorted(required-paired)),flush=True)
print('BINDING CENSUS ONLY; no full-API/HTML/JS/input/HTTPS/phase acceptance claim')
