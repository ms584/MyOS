"""Validate the pinned source-built Cobalt core; build identity never implies acceptance."""
from pathlib import Path
import hashlib,json,subprocess,re

def load_source_core(manifest_path):
 root=Path('/home/xytron404/myos');workspace=root/'build/phase335-core-workspace';source=workspace/'src'
 evidence=root/'docs/wip/phase335/checkpoint7/tooltip-core-build'
 path=Path(manifest_path).resolve();assert path.is_relative_to(workspace.resolve())
 manifest=json.loads(path.read_text());identity=json.loads((evidence/'patch-identity.json').read_text())
 digest=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
 assert manifest['build_completed'] is True and manifest['official_google_binary'] is False
 assert manifest['source_commit']==identity['source_commit']
 assert subprocess.check_output(['git','rev-parse','HEAD'],cwd=source,text=True).strip()==identity['source_commit']
 assert digest(source/identity['path'])==identity['modified_sha256']==manifest['tooltip_source_sha256']
 assert subprocess.check_output(['git','diff','--name-only'],cwd=source,text=True).splitlines()==[identity['path']]
 assert manifest['source_patch_sha256']==digest(evidence/'tooltip-in-root.patch')
 core=Path(manifest['core_path']).resolve()
 assert core==(source/'out/evergreen-x64_gold/libcobalt.so').resolve()
 assert core.stat().st_size==manifest['core_bytes'] and digest(core)==manifest['core_sha256']
 log=root/'build/phase335-core-compile.log'
 assert digest(log)==manifest['build_log_sha256'] and re.search(r'\] exit=0\s*$',log.read_text())
 assert manifest['phase_passed'] is False,'Build identity must never imply acceptance'
 return core,manifest
