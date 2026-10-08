"""Archive only manifest-listed fixture PNGs, never private TLS/build material."""
from pathlib import Path
import hashlib,io,json,tarfile
root=Path('/home/xytron404/native-integrated-XZBHuo/build/native-verified-tls-h35djree')
destination=Path('/home/xytron404/myos/docs/wip/native-pmm512/integration-20261008')
evidence=json.loads((root/'evidence.json').read_text())
names=sorted({item['frame'] for item in evidence['visual']})
assert names
inventory={}
with tarfile.open(destination/'native-frames.tar.gz','w:gz') as archive:
    for name in names:
        path=root/name
        assert path.resolve().parent==root and not path.is_symlink()
        assert name.startswith('launch-') and name.endswith('.png') and '/' not in name
        data=path.read_bytes();assert data.startswith(b'\x89PNG\r\n\x1a\n')
        info=tarfile.TarInfo(name);info.size=len(data);info.mode=0o644;info.mtime=0
        archive.addfile(info,io.BytesIO(data))
        inventory[name]=hashlib.sha256(data).hexdigest()
report=dict(files=inventory,archive_sha256=hashlib.sha256((destination/'native-frames.tar.gz').read_bytes()).hexdigest())
(destination/'native-frames.json').write_text(json.dumps(report,indent=2)+'\n')
with tarfile.open(destination/'native-frames.tar.gz','r:gz') as archive:
    assert archive.getnames()==names
    for item in archive:
        assert item.isfile() and hashlib.sha256(archive.extractfile(item).read()).hexdigest()==inventory[item.name]
print('PASS public PNG-only archive:',len(names),'frames,', (destination/'native-frames.tar.gz').stat().st_size,'bytes; no keys/assets')
