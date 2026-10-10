"""Summarize private encrypted NIC capture. No TLS keys/decryption/media demux.
Commits metadata only: DNS hostnames, endpoint byte counts and transport times.
"""
import argparse,json,struct,collections,hashlib
from pathlib import Path
p=argparse.ArgumentParser(description=__doc__);p.add_argument('pcap',type=Path);a=p.parse_args()
b=a.pcap.read_bytes();assert b[:4] in (b'\xd4\xc3\xb2\xa1',b'\xa1\xb2\xc3\xd4');endian='<' if b[:4]==b'\xd4\xc3\xb2\xa1' else '>'
assert struct.unpack(endian+'I',b[20:24])[0]==1
pos=24;flows={};names=collections.Counter();packets=0;times=[]
while pos+16<=len(b):
    sec,usec,n,wire=struct.unpack_from(endian+'IIII',b,pos);pos+=16;frame=b[pos:pos+n];pos+=n;packets+=1;times.append(sec+usec/1e6)
    if len(frame)<34 or frame[12:14]!=b'\x08\x00':continue
    ip=frame[14:];hlen=(ip[0]&15)*4
    if len(ip)<hlen+8:continue
    proto=ip[9];src='.'.join(map(str,ip[12:16]));dst='.'.join(map(str,ip[16:20]));payload=ip[hlen:];sport,dport=struct.unpack('!HH',payload[:4])
    if proto==6:
        key=f'{src}:{sport}>{dst}:{dport}';r=flows.setdefault(key,dict(packets=0,bytes=0,first_utc_epoch=sec+usec/1e6,last_utc_epoch=0));r['packets']+=1;r['bytes']+=len(payload);r['last_utc_epoch']=sec+usec/1e6
    if proto==17 and (sport==53 or dport==53):
        dns=payload[8:]
        if len(dns)<12:continue
        qd=struct.unpack('!H',dns[4:6])[0];cur=12
        for _ in range(min(qd,4)):
            labels=[]
            while cur<len(dns):
                size=dns[cur];cur+=1
                if size==0:break
                if size>63 or cur+size>len(dns):labels=[];break
                labels.append(dns[cur:cur+size].decode('ascii','replace'));cur+=size
            cur+=4;host='.'.join(labels).lower()
            if host.endswith(('.youtube.com','.googlevideo.com','.ytimg.com','.googleapis.com')):names[host]+=1
result=dict(phase_passed=False,pcap_sha256=hashlib.sha256(b).hexdigest(),pcap_bytes=len(b),packets=packets,first_utc_epoch=min(times),last_utc_epoch=max(times),dns_names=dict(names),tcp_flows=flows,tls_decrypted=False,host_fetch=False,caveat='Encrypted NIC metadata proves network flows, not decrypted HTTP status or certificate validation. Native runtime trust/HTTPS verification uses pinned prerequisites; no credentials or payload exported.')
out=a.pcap.parent/'network-safe.json';out.write_text(json.dumps(result,indent=2)+'\n');print(out,'packets',packets,'Googlevideo names',sum('.googlevideo.com' in x for x in names))
