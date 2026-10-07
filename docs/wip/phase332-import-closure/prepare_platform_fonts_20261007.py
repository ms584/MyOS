"""Package licensed real font files; all parsing/rasterization remains in MyOS."""
import hashlib,json,shutil,sys
from pathlib import Path
root=Path(sys.argv[1]);out=root/'build/platform-fonts-20261007';out.mkdir(parents=True,exist_ok=True)
stage=Path('/mnt/c/Users/Xytron/Documents/Codex/2026-09-30/myos-checkpoint-9f58024-home-xytron404-myos-2/work')
inputs={
 '/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf':'b4c632e3cdf9acc7f28758fb5a323c8524d7fc6660d46904d9b6cbe2809c419c',
 '/usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf':'a54dca07c76d6289e717e75e0a58c0128f6d7269ef3faf76417c9d7d3bba37ab',
 '/usr/share/fonts/truetype/dejavu/DejaVuSerif.ttf':'a52c2a69dfc93d0b511b98c1019a57eb3d6a62d328bdf1387f00851b02e3a315',
 '/usr/share/doc/fonts-dejavu-core/copyright':'63d3ba759d12804c5b31a9d5940d855c1820d1f5999e6b0872eb1c7ff045fbc9'}
records=[]
for original,digest in inputs.items():
 source=Path(original);data=source.read_bytes();assert hashlib.sha256(data).hexdigest()==digest,original
 name='LICENSE.txt' if source.name=='copyright' else source.name
 if name.endswith('.ttf'):assert data[:4]==b'\x00\x01\x00\x00'
 destination=out/name;shutil.copyfile(source,destination)
 records.append(dict(name=name,bytes=len(data),sha256=digest,source=original))
configuration=stage/'platform-fonts-20261007.xml';shutil.copyfile(configuration,out/'fonts.xml')
data=configuration.read_bytes();records.append(dict(name='fonts.xml',bytes=len(data),sha256=hashlib.sha256(data).hexdigest(),source=str(configuration)))
(out/'report.json').write_text(json.dumps(dict(font_package='DejaVu fonts2.37 Ubuntu2.37-8build1',
 license='Bitstream Vera/DejaVu, notice included LICENSE.txt',resources=records),indent=2)+'\n')
print('Prepared native platform font assets:',out,'total bytes',sum(item['bytes'] for item in records),flush=True)
