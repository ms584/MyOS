from pathlib import Path
import json,base64,urllib.parse,hashlib,sys,struct,re
p=Path(sys.argv[1]);t=p.read_text(errors='replace');start=t.find('{"constants":');assert start>=0
c,_=json.JSONDecoder().raw_decode(t[start+len('{"constants":'):]);types={v:k for k,v in c['logEventTypes'].items()};events=[]
for line in t[t.find('"events"',start):].splitlines():
 try:e=json.loads(line.strip().rstrip(','))
 except json.JSONDecodeError:continue
 if isinstance(e,dict) and 'source' in e and 'type' in e:events.append(e)
streams={};sessions={};payloads={};statuses={};received={}
for e in events:
 name=types.get(e['type'],'');sid=e['source']['id'];params=e.get('params',{})
 if name=='HTTP2_SESSION_INITIALIZED':sessions[sid]=params.get('source_dependency',{}).get('id')
 if name=='HTTP2_SESSION_SEND_HEADERS':
  hs=params.get('headers',[]);d={}
  for h in hs:
   if h.startswith(':'):k,_,v=h[1:].partition(': ');d[':'+k]=v
   else:k,_,v=h.partition(': ');d[k.lower()]=v
  host=d.get(':authority','');path=d.get(':path','')
  if host.endswith('.googlevideo.com') and path.startswith('/videoplayback?'):
   streams[(sid,params['stream_id'])]=d
 if name=='HTTP2_SESSION_RECV_HEADERS':
  key=(sid,params.get('stream_id'));statuses.setdefault(key,[]).extend(h.split(': ',1)[1] for h in params.get('headers',[]) if h.startswith(':status: '))
 if name=='HTTP2_SESSION_RECV_DATA':
  key=(sid,params.get('stream_id'));received[key]=received.get(key,0)+params.get('size',params.get('byte_count',0))
 if name=='SSL_SOCKET_BYTES_SENT' and 'bytes' in params:
  payloads.setdefault(sid,bytearray()).extend(base64.b64decode(params['bytes']))
frames={};malformed=[]
for sid,b in payloads.items():
 i=24 if b.startswith(b'PRI * HTTP/2.0\r\n\r\nSM\r\n\r\n') else 0
 while i+9<=len(b):
  n=int.from_bytes(b[i:i+3],'big');kind=b[i+3];flags=b[i+4];stream=int.from_bytes(b[i+5:i+9],'big')&0x7fffffff
  if i+9+n>len(b):break
  payload=b[i+9:i+9+n]
  if kind==0:
   if flags&8:
    padding=payload[0];payload=payload[1:len(payload)-padding]
   frames.setdefault((sid,stream),bytearray()).extend(payload)
  i+=9+n
results=[];private=p.parent/'private-native-post';private.mkdir(exist_ok=True)
# The observed Googlevideo transport uses HTTP/1.1, while page resources use h2.
url_sources={};response_statuses={};response_bytes={}
for e in events:
 sid=e['source']['id'];params=e.get('params',{});name=types.get(e['type'],'')
 if name=='URL_REQUEST_START_JOB' and isinstance(params.get('url'),str):url_sources[sid]=params['url']
 if name=='HTTP_TRANSACTION_READ_RESPONSE_HEADERS':
  hs=params.get('headers',[]);hs='\n'.join(hs) if isinstance(hs,list) else str(hs)
  response_statuses.setdefault(sid,[]).extend(re.findall(r'HTTP/[^ ]+ ([0-9]{3})',hs))
 if name=='URL_REQUEST_JOB_FILTERED_BYTES_READ':response_bytes[sid]=response_bytes.get(sid,0)+params.get('byte_count',0)
for socket,wire in payloads.items():
 if wire.startswith(b'PRI * HTTP/2.0'):continue
 offset=0
 while offset<len(wire):
  end=wire.find(b'\r\n\r\n',offset)
  if end<0:break
  lines=wire[offset:end].decode('latin1').split('\r\n');first=lines[0].split(' ')
  if len(first)!=3 or not first[2].startswith('HTTP/1.'):break
  method,target,version=first;headers={}
  for line in lines[1:]:
   key,sep,value=line.partition(':');
   if sep:headers[key.lower()]=value.strip()
  length=int(headers.get('content-length','0'))
  if headers.get('transfer-encoding','').lower()=='chunked':break
  body_start=end+4;body_end=body_start+length
  if body_end>len(wire):break
  host=headers.get('host','');body=bytes(wire[body_start:body_end]);offset=body_end
  if not host.endswith('.googlevideo.com') or not target.startswith('/videoplayback?') or method!='POST':continue
  uri='https://'+host+target;digest=hashlib.sha256(uri.encode()).hexdigest();matching=[sid for sid,u in url_sources.items() if u==uri]
  statuses=[status for sid in matching for status in response_statuses.get(sid,[])]
  item=dict(transport='HTTP/1.1',socket=socket,method=method,uri_sha256=digest,hostname=host,body_captured=True,body_bytes=len(body),body_sha256=hashlib.sha256(body).hexdigest(),http_status=statuses,received_data_bytes=sum(response_bytes.get(sid,0) for sid in matching),expire=urllib.parse.parse_qs(urllib.parse.urlsplit(uri).query).get('expire',[None])[0])
  headers.update({':method':method,':authority':host,':path':target})
  (private/(digest+'.json')).write_text(json.dumps(dict(uri=uri,headers=headers,body_base64=base64.b64encode(body).decode()),indent=2)+'\n')
  results.append(item)

for (session,stream),headers in streams.items():
 socket=sessions.get(session);body=frames.get((socket,stream));uri='https://'+headers[':authority']+headers[':path'];digest=hashlib.sha256(uri.encode()).hexdigest()
 item=dict(http_status=statuses.get((session,stream),[]),received_data_bytes=received.get((session,stream),0),session=session,socket=socket,stream=stream,method=headers.get(':method'),uri_sha256=digest,hostname=headers[':authority'],body_captured=body is not None,body_bytes=len(body) if body is not None else 0,body_sha256=hashlib.sha256(body).hexdigest() if body is not None else None,expire=urllib.parse.parse_qs(urllib.parse.urlsplit(uri).query).get('expire',[None])[0])
 if body is not None:
  (private/(digest+'.json')).write_text(json.dumps(dict(uri=uri,headers=headers,body_base64=base64.b64encode(body).decode()),indent=2)+'\n')
 results.append(item)
(p.parent/'native-post-safe-analysis.json').write_text(json.dumps(dict(diagnostic_only=True,phase_passed=False,requests=results),indent=2)+'\n')
print(json.dumps(dict(diagnostic_only=True,event_count=len(events),receive_payload_events=sum(types.get(e['type']) in ['SSL_SOCKET_BYTES_RECEIVED','SOCKET_BYTES_RECEIVED'] for e in events),send_payload_events=sum(types.get(e['type'])=='SSL_SOCKET_BYTES_SENT' for e in events),session_bindings=len(sessions),requests=results),indent=2))
