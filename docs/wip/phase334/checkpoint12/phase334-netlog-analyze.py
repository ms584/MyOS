from pathlib import Path
import json,re,hashlib,urllib.parse,sys
p=Path(sys.argv[1]);text=p.read_text(errors='replace');start=text.find('{"constants":');assert start>=0
text=text[start:];constants,_=json.JSONDecoder().raw_decode(text[len('{"constants":'):]);types={v:k for k,v in constants['logEventTypes'].items()}
events=[]
for line in text[text.find('"events"'):].splitlines():
 line=line.strip().rstrip(',')
 if not line.startswith('{'):continue
 try:e=json.loads(line)
 except json.JSONDecodeError:continue
 if 'source' in e and 'type' in e:events.append(e)
sources={}
for e in events:
 sid=e['source']['id'];sources.setdefault(sid,[]).append(e)
media=[]
for sid,es in sources.items():
 urls=[e['params'].get('url','') for e in es if isinstance(e.get('params'),dict) and isinstance(e['params'].get('url'),str)]
 urls=[u for u in urls if (urllib.parse.urlsplit(u).hostname or '').endswith('.googlevideo.com')]
 if not urls:continue
 u=urls[0];parsed=urllib.parse.urlsplit(u);q=urllib.parse.parse_qs(parsed.query)
 item=dict(source_id=sid,uri_sha256=hashlib.sha256(u.encode()).hexdigest(),hostname=parsed.hostname,path=parsed.path,method=None,expire=q.get('expire',[None])[0],event_types=sorted({types.get(e['type'],str(e['type'])) for e in es}),http_status=[],upload_bytes=[])
 for e in es:
  params=e.get('params',{});name=types.get(e['type'],'')
  if 'method' in params:item['method']=params['method']
  if 'headers' in params and 'READ_RESPONSE_HEADERS' in name:
   headers=params['headers'];headers='\n'.join(headers) if isinstance(headers,list) else str(headers)
   item['http_status']+=re.findall(r'HTTP/[^ ]+ ([0-9]{3})',headers)
  if 'UPLOAD' in name and 'byte_count' in params:item['upload_bytes'].append(params['byte_count'])
 media.append(item)
result=dict(diagnostic_only=True,phase_passed=False,export_bytes=len(text),complete_json=False,event_count=len(events),googlevideo_sources=media)
try:json.loads(text[:text.rfind('myos>')].strip());result['complete_json']=True
except (json.JSONDecodeError,ValueError):pass
out=p.parent/'netlog-safe-analysis.json';out.write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result,indent=2))
