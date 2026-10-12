from pathlib import Path
import hashlib,json,struct,os,datetime,shutil
root=Path.cwd();source=root/'build/phase335-absolute-manual';provider=root/'build/media-evaluation/provider-uf3n3z4l';out=root/'build/phase335-handoff-manual'
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
d=json.loads((source/'evidence.json').read_text());r=json.loads((provider/'report.json').read_text())
assert not r['isolated_player_source'] and not r['diagnostic_trace']
assert sha(root/'user/media-player.c')==r['player_source_sha256']
assert sha(provider/'provider.so')==r['provider_sha256']
normal=d['normal_identity'];assert all(sha(root/'build'/name)==value for name,value in normal.items())
assert shutil.disk_usage(root).free>250*1024*1024
assert not out.exists();out.mkdir()
old=source/'assets.img'
with old.open('rb') as inp:
 header=inp.read(512);magic,version,count,total=struct.unpack_from('<16sIIQ',header);assert version==2
 rt_off,rt_size=struct.unpack_from('<QQ',header,32);entries=[]
 for i in range(count):
  inp.seek(512+i*256);name,offset,size=struct.unpack('<240sQQ',inp.read(256))
  entries.append((name.split(bytes([0]))[0].decode(),offset,size))
 assert sum(n=='provider' for n,_,_ in entries)==1
 offset=max(4096,(512+256*count+511)&~511);new=[]
 for name,oldoff,size in entries:
  nsize=(provider/'provider.so').stat().st_size if name=='provider' else size
  new.append((name,oldoff,size,offset,nsize));offset=(offset+nsize+511)&~511
 new_rt=offset;offset=(offset+rt_size+511)&~511;new_total=max(1024*1024,offset)
 hashes={}
 with (out/'assets.img').open('xb') as dst:
  dst.truncate(new_total);dst.write(struct.pack('<16sIIQQQ',magic,version,count,new_total,new_rt,rt_size))
  dst.seek(512)
  for name,oldoff,size,pos,nsize in new:dst.write(struct.pack('<240sQQ',name.encode(),pos,nsize))
  for name,oldoff,size,pos,nsize in new:
   dst.seek(pos);h=hashlib.sha256()
   if name=='provider':
    data=(provider/'provider.so').read_bytes();dst.write(data);h.update(data)
   else:
    inp.seek(oldoff);remaining=size
    while remaining:
     data=inp.read(min(remaining,1024*1024));assert data;dst.write(data);h.update(data);remaining-=len(data)
   hashes[name]=h.hexdigest()
  inp.seek(rt_off);runtime=inp.read(rt_size);assert hashlib.sha256(runtime).hexdigest()==normal['platform-runtime.mrt']
  dst.seek(new_rt);dst.write(runtime)
assert hashes['provider']==r['provider_sha256'] and hashes['cobalt']==d['core_sha256']
for name in ('boot.img','platform-runtime.mrt'):os.link(source/name,out/name)
frozen={name:sha(out/name) for name in ('boot.img','assets.img','platform-runtime.mrt')}
d.update(date=datetime.datetime.now().astimezone().isoformat(),provider_sha256=r['provider_sha256'],candidate_only=False,diagnostic_only=False,prepared_only=True,production_player_source_sha256=r['player_source_sha256'],phase_passed=False,observation_only=True,normal_playback_acceptance=False)
d['normal_identity_after']={name:sha(root/'build'/name) for name in normal};assert d['normal_identity_after']==normal
d['provenance']='Production source handoff repair; same frozen normal kernel/runtime/source-built Cobalt and resources; provider from user/media-player.c without trace or isolated source.'
(out/'evidence.json').write_text(json.dumps(d,indent=2)+chr(10))
(out/'manual-assets-identity.json').write_text(json.dumps(frozen,indent=2)+chr(10))
record={'bundle':str(out),'date':d['date'],'normal_identity':normal,'frozen_inputs':frozen,'provider_report':str(provider/'report.json'),'provider_sha256':r['provider_sha256'],'production_player_source_sha256':r['player_source_sha256'],'source_core_sha256':d['core_sha256'],'entries_sha256':hashes,'assets_bytes':new_total,'provider_only_repacked':True,'other_inputs_reused':True,'phase_passed':False}
(root/'docs/wip/phase335/checkpoint22/normal-handoff-bundle.json').write_text(json.dumps(record,indent=2)+chr(10))
print('Prepared production normal bundle',out,'assets bytes',new_total,'phase acceptance pending.')
