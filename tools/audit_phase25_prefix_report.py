"""Offline audit of owner readback; no physical device access or writes."""
from pathlib import Path
import json,hashlib,base64,struct,sys
ROOT=Path(__file__).resolve().parents[1]
IMAGE=ROOT/'build/phase24-usb-bot-clock-2026-10-24/images/myos-usb-data.img'
def sha(x):return hashlib.sha256(x).hexdigest()
def audit(d,prior):
 r=IMAGE.read_bytes();assert sha(r)==d['reference_image_sha256']==prior['reference_image_sha256']=='d66310b71d0ecb737941bf5a6dfb22448cf728d0211456de923a06d0effa85aa'
 assert d['physical'] is True and prior['physical'] is True
 assert d['device']==prior['device'] and d['device']['FriendlyName']=='Samsung Flash Drive DUO'
 assert d['chunk_bytes']==65536 and d['chunks_checked']==544 and not d['capture_truncated']
 assert d['captured_chunks']==len(d['changed_chunks'])==4
 a=bytearray(r);seen=set()
 for c in d['changed_chunks']:
  o=c['offset'];assert o%65536==0 and 0<=o<35651584 and o not in seen;seen.add(o)
  raw=base64.b64decode(c['content_base64'],validate=True)
  assert len(raw)==c['bytes']==65536 and sha(raw)==c['actual_sha256'] and sha(r[o:o+65536])==c['expected_sha256']
  a[o:o+65536]=raw
 assert sha(a[:35651584])==prior['regions'][0]['actual_sha256']=='086eb42035aec728f98c1b00af5ccb0b9673fcdf0f4c3bc24e7cf0954885b90f'
 assert [x['match'] for x in prior['regions']]==[False,True,True]
 for c in prior['regions'][1:]:assert sha(r[c['offset']:c['offset']+c['bytes']])==c['expected_sha256']==c['actual_sha256']
 start=1048576;bps=struct.unpack_from('<H',r,start+11)[0];spc=r[start+13];reserved=struct.unpack_from('<H',r,start+14)[0];nf=r[start+16];entries=struct.unpack_from('<H',r,start+17)[0];fatsz=struct.unpack_from('<H',r,start+22)[0]
 assert (bps,spc,reserved,nf,entries,fatsz)==(512,4,1,2,512,64)
 fat=start+reserved*bps;root=fat+nf*fatsz*bps;data=root+entries*32;clusterbytes=bps*spc
 def directory(off,length):
  found=[];lfn=[]
  for o in range(off,off+length,32):
   e=bytes(a[o:o+32])
   if not e[0]:break
   assert e[0]!=229
   if e[11]==15:
    lfn.append((e[0]&31,e[1:11]+e[14:26]+e[28:32]));continue
   name=b''.join(x[1] for x in sorted(lfn)).decode('utf-16-le').split('\x00')[0] if lfn else e[:11].decode('ascii').strip()
   found.append((name,struct.unpack_from('<H',e,26)[0],struct.unpack_from('<I',e,28)[0],e[11]));lfn=[]
  return found
 rootitems=directory(root,entries*32)
 assert rootitems==[('EFI',2,0,16),('MYOS EFI',0,0,8),('System Volume Information',166,0,22)],rootitems
 svi=directory(data+(166-2)*clusterbytes,clusterbytes)
 assert [(n,c,s) for n,c,s,attr in svi]==[('.',166,0),('..',0,0),('WPSettings.dat',167,12),('IndexerVolumeGuid',168,76)],svi
 allowed=[(440,444),(root+64,root+160),(data+164*clusterbytes,data+167*clusterbytes)]
 for copy in range(nf):
  for cl in [166,167,168]:
   o=fat+copy*fatsz*bps+cl*2
   assert r[o:o+2]==b'\x00\x00' and a[o:o+2]==b'\xff\xff';allowed.append((o,o+2))
 assert not any(r[root+64:root+160]) and not any(r[data+164*clusterbytes:data+167*clusterbytes])
 diffs=[i for i in range(35651584) if r[i]!=a[i]]
 assert len(diffs)==291 and all(any(lo<=i<hi for lo,hi in allowed) for i in diffs)
 # Byte equality outside narrowly validated additions proves existing EFI files,
 # loader/kernel bytes, boot code, partition table, BPB and existing FAT chains unchanged.
 normalized=bytearray(a)
 for lo,hi in allowed:normalized[lo:hi]=r[lo:hi]
 assert normalized==r
 return {'result':'PASS selected-media content integrity; original raw checksum FAIL retained','changed_bytes':len(diffs),'changed_chunks':len(seen),'reconstructed_prefix_sha256':sha(a[:35651584]),'normalized_boot_prefix_sha256':sha(normalized[:35651584]),'reference_boot_prefix_sha256':sha(r[:35651584]),'reference_image_sha256':sha(r),'existing_boot_code_partition_table_bpb_efi_files_existing_fat_chains':'BYTE IDENTICAL','explained_additions':{'mbr_disk_signature_bytes':[440,444],'root_directory':'System Volume Information','new_previously_free_clusters':[166,167,168],'files':['WPSettings.dat','IndexerVolumeGuid'],'fat_copies':2},'cause':'Observed additions are Windows volume metadata; Windows origin inferred from names/content and owner Windows readback context. No existing MyOS boot content changed.','scope':'Selected ThinkBook offline profile only; not raw byte identity after host mounting; no physical write or disk access by this audit','physical_evidence':'Owner-run read-only prefix diagnostic plus previous marker/tail digests; exact reconstructed prefix digest agrees with independent prior physical readback.'}
if __name__=='__main__':
 d=json.loads(Path(sys.argv[1]).read_bytes());prior=json.loads(Path(sys.argv[2]).read_bytes())
 print(json.dumps(audit(d,prior),indent=2))