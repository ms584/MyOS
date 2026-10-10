"""Scoped native expiry/recovery audit. No host media fetching or codec decoding.
Human inspection of actual YouTube recovery frames remains mandatory.
"""
import argparse,datetime,email.utils,hashlib,json,math,re,wave
from pathlib import Path
from PIL import Image,ImageChops
ap=argparse.ArgumentParser(description=__doc__)
ap.add_argument('bundle',type=Path)
ap.add_argument('--expected',type=Path,required=True)
ap.add_argument('--motion-start',type=float,required=True)
ap.add_argument('--motion-end',type=float,required=True)
a=ap.parse_args();d=json.loads((a.bundle/'evidence.json').read_text());expected=json.loads(a.expected.read_text())
reports=json.loads((a.bundle/'safe-native-responses.json').read_text());serial=(a.bundle/'serial.log').read_text()
assert d['normal_identity']==d['normal_identity_after']
assert not d['host_fetch'] and not d['host_decode'] and not d['media_hostname_mapping']
assert d['method']=='POST' and d['request_body_bytes']==expected['body_bytes']
assert d['uri_sha256']==expected['uri_sha256'] and d['body_sha256']==expected['body_sha256']
assert any(r['kind']=='native-body-sha256' and r['value']==expected['body_sha256'] for r in reports)
assert not any(x in serial for x in ['PANIC','USER FAULT','USERFAULT'])
expiry=datetime.datetime.fromisoformat(d['expiry_utc'])
after=[r for r in reports if r['kind'].startswith('after-expiry-')]
assert after and all(datetime.datetime.fromisoformat(r['utc'].replace('Z','+00:00'))>=expiry for r in after)
assert any(r['kind']=='restart' and r['value']=='ordinary-watch' for r in reports)
# NetLog records are interpreted as data. Private URLs/headers never enter the report.
text=(a.bundle/'private-native-netlog-export.txt').read_text()
start=text.find('{"constants":');assert start>=0
constants,_=json.JSONDecoder().raw_decode(text[start+len('{"constants":'):])
types={v:k for k,v in constants['logEventTypes'].items()};events=[]
for line in text[text.find('"events"',start):].splitlines():
 try:e=json.loads(line.strip().rstrip(','))
 except json.JSONDecodeError:continue
 if isinstance(e,dict) and 'source' in e and 'type' in e:events.append(e)
post={};statuses={};response_dates={}
for e in events:
 sid=e['source']['id'];params=e.get('params',{});name=types.get(e['type'],'')
 if name=='URL_REQUEST_START_JOB' and params.get('method')=='POST' and isinstance(params.get('url'),str):
  if hashlib.sha256(params['url'].encode()).hexdigest()==expected['uri_sha256']:
   post[sid]=dict(source_id=sid,method='POST',uri_sha256=expected['uri_sha256'],native_mono_ms=int(e['time']))
 if name=='HTTP_TRANSACTION_READ_RESPONSE_HEADERS':
  hs=params.get('headers',[]);hs='\n'.join(hs) if isinstance(hs,list) else str(hs)
  statuses.setdefault(sid,[]).extend(int(x) for x in re.findall(r'HTTP/[^ ]+ ([0-9]{3})',hs))
  for header in hs.splitlines():
   if header.lower().startswith('date: '):
    response_dates.setdefault(sid,[]).append(email.utils.parsedate_to_datetime(header[6:]).isoformat())
requests=sorted(post.values(),key=lambda x:x['native_mono_ms'])
assert len(requests)>=2
for r in requests:r.update(statuses=statuses.get(r['source_id'],[]),server_dates=response_dates.get(r['source_id'],[]))
before=[r for r in requests if 200 in r['statuses']];expired=[r for r in requests if 403 in r['statuses']]
assert before and expired and min(r['native_mono_ms'] for r in expired)>min(r['native_mono_ms'] for r in before)
assert all(r['server_dates'] and all(datetime.datetime.fromisoformat(t)>=expiry for t in r['server_dates']) for r in expired)
roi=(40,80,740,350);fs=[f for f in d['frames'] if a.motion_start<=f['elapsed']<=a.motion_end]
assert len(fs)>=7 and fs[-1]['elapsed']-fs[0]['elapsed']>=30
changes=[]
for x,y in zip(fs,fs[1:]):
 im1=Image.open(a.bundle/x['path']).convert('RGB').crop(roi);im2=Image.open(a.bundle/y['path']).convert('RGB').crop(roi)
 diff=ImageChops.difference(im1,im2);pixels=diff.get_flattened_data() if hasattr(diff,'get_flattened_data') else diff.getdata()
 changed=sum(p!=(0,0,0) for p in pixels);assert changed>500
 changes.append(dict(before=x['path'],after=y['path'],changed_pixels=changed))
with wave.open(str(a.bundle/'audio.wav'),'rb') as w:
 assert (w.getframerate(),w.getnchannels(),w.getsampwidth())==(11025,1,1)
 pcm=w.readframes(w.getnframes())
rms=[math.sqrt(sum((x-128)**2 for x in pcm[i:i+11025])/11025) for i in range(0,len(pcm)-11025+1,11025)]
assert sum(x>1 for x in rms)>=30
assert len(d['qemu'])==1 and d['desktop_responsive'] and d['normal_resources_restored']
assert d['before']==d['after']==d['after_desktop']
assert 'WINDOW OPEN 1\nDESKTOP FRAME\n' in serial and serial.rstrip().endswith('myos>')
metrics=[dict((k,int(v)) for k,v in re.findall(r'([\w-]+)=(-?\d+)',line)) for line in serial.splitlines() if line.startswith('MEDIA metrics ')]
hardware=[dict((k,int(v)) for k,v in re.findall(r'([\w-]+)=(-?\d+)',line)) for line in serial.splitlines() if line.startswith('MEDIA hardware-clock ')]
assert metrics and hardware and all(x['failures']==0 for x in hardware)
assert all(m['corrupted']==0 for m in metrics)
assert all(m['max-sync-us']<=100000 for m in metrics), 'actual native A/V sync exceeds the declared 100ms acceptance bound'
report=dict(phase_passed=False,scoped_native_expired_uri_error_and_recovery=True,human_youtube_frame_review_required=True,
 uri_sha256=expected['uri_sha256'],body_sha256=expected['body_sha256'],body_bytes=expected['body_bytes'],expiry_utc=d['expiry_utc'],
 native_requests=requests,native_body_sha_confirmed=True,native_after_reports=after,normal_identity_exact=True,desktop_shell_resources_exact=True,
 recovered_motion_span_seconds=fs[-1]['elapsed']-fs[0]['elapsed'],recovered_motion_roi=roi,recovered_motion_changes=changes,
 native_pcm_sha256=hashlib.sha256((a.bundle/'audio.wav').read_bytes()).hexdigest(),native_pcm_seconds=len(pcm)/11025,native_pcm_audible_second_bins=sum(x>1 for x in rms),
 actual_hardware_query_failures=0,observed_max_sync_us=max(m['max-sync-us'] for m in metrics),owned_mapping_peak_bytes=max(m['ram-query-bytes'] for m in metrics),
 scope='Native peer HTTP200 before and HTTP403 after real expiry for same URI/body; browser status0 may be CORS opaque. Native display/PCM recovery, exact close/desktop resources. Never whole33.4/33.5/33/Rickroll acceptance by this script alone.')
(a.bundle/'native-expiry-recovery-analysis.json').write_text(json.dumps(report,indent=2)+'\n')
print('PASS scoped native real-expiry peer error/recovery audit; whole-phase/human review still required')
