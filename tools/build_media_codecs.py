"""Build pinned target-only VP8/Opus archives; NOT playback evidence."""
from pathlib import Path
import argparse,hashlib,json,subprocess,shutil
p=argparse.ArgumentParser(description=__doc__)
p.add_argument("--vp9",action="store_true",help="Separate bounded VP8/VP9 target archive; does not replace VP8 baseline")
args=p.parse_args()
root=Path(__file__).resolve().parents[1]
base=root/'build/media-evaluation'
lock=json.loads((root/'docs/PHASE333_MEDIA_LOCK.json').read_text())
pins={item['archive']:item['sha256'] for item in lock['dependencies']}
import tarfile
for name,digest in pins.items():
    archive=base/name
    assert hashlib.sha256(archive.read_bytes()).hexdigest()==digest,name
    with tarfile.open(archive) as tar:
        members=tar.getmembers()
        for member in members:
            assert member.isdir() or member.isfile(),('Unsupported archive member',member.name)
            path=(base/member.name).resolve()
            assert path.is_relative_to(base.resolve()),member.name
            if not member.isfile():continue
            original=tar.extractfile(member).read()
            patched=original
            if member.name in ('libvpx-1.15.2/vp8/decoder/onyxd_if.c','libvpx-1.15.2/vp9/decoder/vp9_decoder.c'):
                old=b'#include "vpx_ports/vpx_timer.h"'
                assert original.count(old)==1
                patched=original.replace(old,b'/* MyOS: unused timer header omitted. */')
            if not path.exists():
                path.parent.mkdir(parents=True,exist_ok=True);path.write_bytes(original);path.chmod(member.mode & 0o777)
            assert path.read_bytes() in (original,patched),('Unpinned source change',member.name)
support=json.loads((root/'build/renderer-provider-evaluation/provider-84sokh6g/report.json').read_text())
math_archive=next(Path(x['path']) for x in support['inputs'] if Path(x['path']).name=='libmyos-math.a')
math_pin=next(x['sha256'] for x in support['inputs'] if Path(x['path'])==math_archive)
assert hashlib.sha256(math_archive.read_bytes()).hexdigest()==math_pin
p=math_archive.parent/'sdk/include/math.h'
assert p.is_file()
dest=base/'math-include'
if not dest.exists(): shutil.copytree(p.parent,dest)
for header in p.parent.rglob('*'):
    if header.is_file():assert (dest/header.relative_to(p.parent)).read_bytes()==header.read_bytes(),header
cc=base/'target-cc'
cc.write_text('#!/bin/sh\nexec python3 "'+str(root/'tools/media_target_cc.py')+'" "$@"\n')
cc.chmod(0o755)
def run(args,cwd=None):
    print('$ '+' '.join(map(str,args)),flush=True)
    subprocess.run(list(map(str,args)),cwd=cwd,check=True)
vpx=base/'libvpx-1.15.2'
# Unused timer include pulls sys/time.h even though no timer is used.
for relative in ('vp8/decoder/onyxd_if.c','vp9/decoder/vp9_decoder.c'):
    p=vpx/relative;s=p.read_text(); old='#include "vpx_ports/vpx_timer.h"'
    assert s.count(old) in (0,1)
    if old in s:p.write_text(s.replace(old,'/* MyOS: unused timer header omitted. */'))
out=base/('vpx-target-vp9' if args.vp9 else 'vpx-target');out.mkdir(exist_ok=True)
if not (out/'Makefile').exists():
    codec_flags=['--enable-vp9','--disable-vp9-encoder','--disable-vp9-highbitdepth','--size-limit=640x360'] if args.vp9 else ['--disable-vp9']
    run([vpx/'configure','--target=generic-gnu',*codec_flags,'--disable-vp8-encoder',
         '--disable-multithread','--disable-runtime-cpu-detect',
         '--disable-examples','--disable-tools','--disable-docs','--disable-unit-tests',
         '--enable-pic','--disable-shared'],out)
run(['make','-j4','CC='+str(cc),'CXX=false','libvpx.a'],out)
opus=base/'opus-1.5.2';opout=base/'opus-target'
# Upstream source groups, no host configure probes or target executables.
import re
from concurrent.futures import ThreadPoolExecutor
opout.mkdir(exist_ok=True)
(opout/'config.h').write_text('#define OPUS_BUILD 1\n#define FIXED_POINT 1\n#define DISABLE_FLOAT_API 1\n#define VAR_ARRAYS 1\n#define ENABLE_HARDENING 1\n#define PACKAGE_VERSION "1.5.2"\n')
groups={}
for manifest in ('opus_sources.mk','celt_sources.mk','silk_sources.mk'):
    text=(opus/manifest).read_text().replace('\\\n',' ')
    for line in text.splitlines():
        if '=' in line:
            key,value=line.split('=',1);groups[key.strip()]=value.split()
sources=sum((groups[key] for key in ('OPUS_SOURCES','CELT_SOURCES','SILK_SOURCES','SILK_SOURCES_FIXED')),[])
assert sources and all((opus/p).is_file() for p in sources)
def compile_source(name):
    obj=opout/(name.replace('/','-')+'.o')
    run([cc,'-std=c99','-O2','-DHAVE_CONFIG_H=1',
         '-I'+str(opout),'-I'+str(opus/'include'),'-I'+str(opus/'celt'),
         '-I'+str(opus/'silk'),'-I'+str(opus/'silk/fixed'),
         '-c',opus/name,'-o',obj])
    return obj
with ThreadPoolExecutor(max_workers=4) as pool:
    objects=list(pool.map(compile_source,sources))
run(['ar','rcs',opout/'libopus.a',*objects])
report={'target':'MyOS x86_64 SDK only','host_decode_executed':False,
        'native_decode_executed':False,'vp9':args.vp9,'decode_size_limit':'640x360' if args.vp9 else None,'pins':pins,'math_archive_sha256':math_pin,'archives':{}}
for path in (out/'libvpx.a',opout/'libopus.a'):
    (path.parent/'undefined.txt').write_text(subprocess.check_output(['nm','-u',str(path)],text=True))
    report['archives'][str(path.relative_to(root))]={'sha256':hashlib.sha256(path.read_bytes()).hexdigest(),'bytes':path.stat().st_size}
(base/('codec-build-vp9.json' if args.vp9 else 'codec-build.json')).write_text(json.dumps(report,indent=2)+'\n')
print(json.dumps(report,indent=2))
