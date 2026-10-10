"""Freeze a completed source-core build for private native tooltip retesting.
This records identity only, never playback, audio or Phase acceptance.
"""
from pathlib import Path
import subprocess,hashlib,json,datetime,re
root=Path('/home/xytron404/myos');workspace=root/'build/phase335-core-workspace';source=workspace/'src'
evidence=root/'docs/wip/phase335/checkpoint7/tooltip-core-build'
sha=lambda path:hashlib.sha256(path.read_bytes()).hexdigest()
identity=json.loads((evidence/'patch-identity.json').read_text())
log=root/'build/phase335-core-compile.log'
assert re.search(r'\] exit=0\s*$',log.read_text()),'Compile has not completed successfully; do not freeze an incomplete/stale core'
head=subprocess.check_output(['git','rev-parse','HEAD'],cwd=source,text=True).strip()
assert head==identity['source_commit']
assert sha(source/identity['path'])==identity['modified_sha256']
assert subprocess.check_output(['git','diff','--name-only'],cwd=source,text=True).splitlines()==[identity['path']]
core=source/'out/evergreen-x64_gold/libcobalt.so'
assert core.exists()
header=subprocess.check_output(['readelf','-h',str(core)],text=True)
assert 'ELF64' in header and 'Advanced Micro Devices X86-64' in header and 'DYN' in header
notes=subprocess.check_output(['readelf','-n',str(core)],text=True)
dynamic=subprocess.check_output(['readelf','-d',str(core)],text=True)
for path in ['build/util/LASTCHANGE','gpu/config/gpu_lists_version.h','skia/ext/skia_commit_hash.h']:
 text=(source/path).read_text();assert '0000000000000000000000000000000000000000' not in text,path
for name,digest in json.loads((evidence/'normal-base-identity.json').read_text()).items():assert sha(root/'build'/name)==digest,name
manifest=dict(build_completed=True,phase_passed=False,runtime_tested=False,source_commit=head,source_patch_sha256=sha(evidence/'tooltip-in-root.patch'),tooltip_source_sha256=sha(source/identity['path']),core_path=str(core),core_sha256=sha(core),core_bytes=core.stat().st_size,build_log_sha256=sha(log),build_configuration=(source/'out/evergreen-x64_gold/args.gn').read_text(),readelf_header=header,readelf_notes=notes,readelf_dynamic=dynamic,dependency_pins=json.loads((evidence/'dependency-pins.json').read_text()),date=datetime.datetime.now(datetime.timezone(datetime.timedelta(hours=7))).isoformat(),host_playback_used=False,official_google_binary=False)
output=workspace/'core-build-identity.json';output.write_text(json.dumps(manifest,indent=2)+'\n');print('Frozen build identity only:',output,'SHA256',manifest['core_sha256'])
