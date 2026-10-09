"""Link native media into the hash-verified EGL/GLES paired provider."""
from pathlib import Path
import argparse,hashlib,json,re,subprocess,tempfile
root=Path(__file__).resolve().parents[1]
p=argparse.ArgumentParser(description=__doc__)
p.add_argument('--renderer',type=Path,default=root/'build/renderer-provider-evaluation/provider-84sokh6g')
p.add_argument('--player-source',type=Path,help='Isolated diagnostic player source inside build/media-evaluation')
p.add_argument('--trace',action='store_true',help='Diagnostic-only player trace; not timing acceptance')
p.add_argument('--punchout-diagnostic',action='store_true',help='Diagnostic output-mode/geometry observation; does not render an underlay yet')
p.add_argument('--native-underlay',action='store_true',help='Experimental native compositor output; requires candidate syscall60')
p.add_argument('--vp9',action='store_true',help='Candidate bounded native VP9, requires separate audited codec build')
args=p.parse_args();source=args.renderer.resolve()
player_source=(args.player_source or root/'user/media-player.c').resolve()
if args.player_source:assert player_source.is_relative_to((root/'build/media-evaluation').resolve())
assert source.is_relative_to((root/'build/renderer-provider-evaluation').resolve())
report=json.loads((source/'report.json').read_text())
digest=lambda path:hashlib.sha256(path.read_bytes()).hexdigest()
assert digest(source/'provider.so')==report['provider_sha256'] and report['paired_exit_abi']==1
base=root/'build/media-evaluation';out=Path(tempfile.mkdtemp(prefix='provider-',dir=base))
inputs=[]
for item in report['inputs']:
 path=Path(item['path'])
 if path.suffix=='.a':
  assert path.resolve().is_relative_to((root/'build').resolve()) and digest(path)==item['sha256']
  inputs.append(path)
objects=[]
for item in report['linked_objects']:
 path=Path(item['path'])
 assert path.resolve().is_relative_to((root/'build').resolve()) and digest(path)==item['sha256']
 objects.append(path)
lock=json.loads((root/'docs/PHASE331_ABI_SOURCES.json').read_text())
gl=root/'build/engine-evaluation/contracts/gles.h'
assert digest(gl)==next(x['sha256'] for x in lock['headers'] if x['path']=='starboard/gles.h')
headers=out/'include/starboard';headers.mkdir(parents=True)
(headers/'gles.h').write_bytes(gl.read_bytes())
configuration=(root/'user/starboard-system/include/starboard/configuration.h').read_text()
configuration=configuration.replace('#define SB_API_VERSION 18','#define SB_API_VERSION 18\n#define SB_SIZE_OF_POINTER 8\n#define SB_SIZE_OF_LONG 8\n#define SB_SIZE_OF(x) SB_SIZE_OF_##x')
(headers/'configuration.h').write_text(configuration)
cc=base/'target-cc'
flags=['-std=c11','-O2','-Wall','-Wextra','-Werror','-Wno-misleading-indentation',
 '-Iuser','-Iinclude','-I'+str(out/'include'),'-Iuser/starboard-media/include',
 '-Iuser/starboard-system/include','-I'+str(base/'libvpx-1.15.2'),
 '-I'+str(base/'opus-1.5.2/include')]
if args.vp9:flags.append('-DMYOS_MEDIA_VP9=1')
if args.trace:flags.append('-DMYOS_MEDIA_TRACE=1')
if args.punchout_diagnostic:flags.append('-DMYOS_MEDIA_PUNCHOUT_DIAGNOSTIC=1')
if args.native_underlay:flags.append('-DMYOS_MEDIA_NATIVE_UNDERLAY=1')
for name in ('media-player','media-capability'):
 obj=out/(name+'.o')
 subprocess.run([str(cc),*flags,'-c',str(player_source if name=='media-player' else root/'user'/(name+'.c')),'-o',str(obj)],check=True)
 objects.append(obj)
codecs=json.loads((base/('codec-build-vp9.json' if args.vp9 else 'codec-build.json')).read_text())
for name,item in codecs['archives'].items():
 path=root/name;assert digest(path)==item['sha256'];inputs.append(path)
library=out/'provider.so'
link=['ld','-shared','--gc-sections','--hash-style=both','-Bsymbolic',
 '-z','now','-z','relro','-z','noexecstack','-u','SbGetEglInterface',
 '-u','SbGetGlesInterface','-u','MyOSPairedExitAbi','-u','MyOSMediaProviderAbi',
 '-o',str(library),*map(str,objects),'--start-group',*map(str,inputs),'--end-group']
with (out/'link.log').open('w') as stream:
 stream.write('$ '+' '.join(link)+'\n');stream.flush()
 subprocess.run(link,stdout=stream,stderr=subprocess.STDOUT,check=True)
dynamic=subprocess.check_output(['readelf','-dW',str(library)],text=True)
undefined=subprocess.check_output(['nm','-u',str(library)],text=True)
symbols=subprocess.check_output(['readelf','--dyn-syms','-W',str(library)],text=True)
assert '(NEEDED)' not in dynamic and '(TEXTREL)' not in dynamic
allowed=set(re.findall(r'\bX\((\w+),',(root/'build/platform-runtime-exports.h').read_text()))
strong={line.split()[-1] for line in undefined.splitlines() if line.split()[0]=='U'}
assert strong<=allowed,sorted(strong-allowed)
(out/'undefined.txt').write_text(undefined);(out/'symbols.txt').write_text(symbols)
result={'vp9_candidate':args.vp9,'capability_source_sha256':digest(root/'user/media-capability.c'),'isolated_player_source':bool(args.player_source),'player_source_sha256':digest(player_source),'experimental_native_underlay':args.native_underlay,'diagnostic_punchout':args.punchout_diagnostic,'diagnostic_trace':args.trace,'provider_sha256':digest(library),'bytes':library.stat().st_size,
 'renderer_provider_sha256':report['provider_sha256'],'media_abi':1,'paired_exit_abi':1,
 'target_only':True,'native_player_executed':False,'cobalt_media_executed':False,
 'inputs':[{'path':str(x),'sha256':digest(x)} for x in inputs],
 'linked_objects':[{'path':str(x),'sha256':digest(x)} for x in objects]}
(out/'report.json').write_text(json.dumps(result,indent=2)+'\n')
print('COMPILE/LINK only; native media acceptance pending',out,flush=True)
