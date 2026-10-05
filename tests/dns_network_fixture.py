"""Deterministic Ethernet DHCP/ARP/DNS peer for native MyOS tests, NOT Internet.

QEMU socket NIC exchanges big-endian length-prefixed Ethernet frames. No host
DNS configuration or browser is used. Only controlled fixture names resolve.
"""
import socket
import struct
import threading
import time


def checksum(data):
    if len(data)&1:data+=b'\0'
    total=sum(struct.unpack('!'+str(len(data)//2)+'H',data))
    while total>>16:total=(total&65535)+(total>>16)
    return (~total)&65535


class DnsNetwork:
    mac=bytes.fromhex('525400123457')
    guest=bytes([10,0,2,15])
    gateway=bytes([10,0,2,2])
    dns=bytes([10,0,2,3])

    def __init__(self):
        self.listener=socket.socket()
        self.listener.bind(('127.0.0.1',0));self.listener.listen(1)
        self.listener.settimeout(.2)
        self.nic=f'socket,model=rtl8139,connect=127.0.0.1:{self.listener.getsockname()[1]}'
        self.stop=threading.Event();self.lock=threading.Lock()
        self.connection=None;self.errors=[];self.queries=[];self.tcp={}
        self.thread=threading.Thread(target=self.run,daemon=True);self.thread.start()

    def send(self,frame):
        assert 14<=len(frame)<=1514
        with self.lock:
            if not self.stop.is_set():self.connection.sendall(struct.pack('!I',len(frame))+frame)

    def ipv4(self,mac,source,target,protocol,payload):
        header=bytearray(struct.pack('!BBHHHBBH4s4s',0x45,0,20+len(payload),1,0x4000,64,protocol,0,source,target))
        struct.pack_into('!H',header,10,checksum(bytes(header)))
        self.send(mac+self.mac+b'\x08\x00'+header+payload)

    def udp(self,mac,source,target,src,dst,payload):
        packet=bytearray(struct.pack('!HHHH',src,dst,len(payload)+8,0)+payload)
        pseudo=source+target+bytes([0,17])+struct.pack('!H',len(packet))
        struct.pack_into('!H',packet,6,checksum(pseudo+packet) or 65535)
        self.ipv4(mac,source,target,17,packet)

    def tcp_packet(self,state,flags,payload=b''):
        packet=bytearray(struct.pack('!HHIIBBHHH',53,state['port'],state['send'],state['receive'],0x50,flags,8192,0,0)+payload)
        pseudo=self.dns+state['ip']+bytes([0,6])+struct.pack('!H',len(packet))
        struct.pack_into('!H',packet,16,checksum(pseudo+packet))
        self.ipv4(state['mac'],self.dns,state['ip'],6,packet)
        state['send']+=len(payload)+bool(flags&2)+bool(flags&1)

    @staticmethod
    def question(packet):
        assert len(packet)>=17
        pos=12;labels=[]
        while packet[pos]:
            count=packet[pos];assert count<=63 and pos+1+count<len(packet)
            labels.append(packet[pos+1:pos+1+count].decode('ascii').lower());pos+=1+count
        pos+=1;assert pos+4==len(packet) and packet[pos:]==b'\0\1\0\1'
        return '.'.join(labels)

    def answer(self,query,truncated=False):
        name=self.question(query);answer=bytearray(query)
        struct.pack_into('!H',answer,2,0x8380 if truncated else 0x8180)
        if truncated:return bytes(answer)
        if name=='missing.test':
            struct.pack_into('!H',answer,2,0x8183);return bytes(answer)
        records=[]
        if name=='cname.test':
            target=b'\x06target\x04test\0';offset=len(answer)+12
            records.append(b'\xc0\x0c'+struct.pack('!HHIH',5,1,60,len(target))+target)
            records.append(struct.pack('!H',0xc000|offset)+struct.pack('!HHIH',1,1,60,4)+bytes([192,0,2,10]))
        else:
            records.append(b'\xc0\x0c'+struct.pack('!HHIH',1,1,60,4)+bytes([192,0,2,10]))
            if name=='multi.test':records.append(b'\xc0\x0c'+struct.pack('!HHIH',1,1,60,4)+bytes([192,0,2,11]))
        struct.pack_into('!H',answer,6,len(records))
        return bytes(answer)+b''.join(records)

    def handle(self,frame):
        assert len(frame)>=14
        kind=frame[12:14];mac=frame[6:12]
        if kind==b'\x08\x06':
            if len(frame)<42 or frame[20:22]!=b'\0\1':return
            requested=frame[38:42]
            if requested not in (self.gateway,self.dns):return
            self.send(mac+self.mac+b'\x08\x06'+struct.pack('!HHBBH',1,0x800,6,4,2)+self.mac+requested+mac+frame[28:32])
            return
        if kind!=b'\x08\x00' or len(frame)<34:return
        header=frame[14:];ihl=(header[0]&15)*4;total=struct.unpack_from('!H',header,2)[0]
        assert ihl>=20 and total<=len(header) and not checksum(header[:ihl])
        payload=header[ihl:total];source=header[12:16];target=header[16:20]
        if header[9]==17:
            src,dst,length,check=struct.unpack_from('!HHHH',payload)
            assert length==len(payload)
            if check:assert not checksum(source+target+bytes([0,17])+struct.pack('!H',length)+payload)
            data=payload[8:]
            if dst==67:
                assert len(data)>=240 and data[:3]==bytes([1,1,6])
                options=data[240:];index=0;message=None
                while index<len(options):
                    code=options[index];index+=1
                    if code==255:break
                    if not code:continue
                    count=options[index];index+=1;value=options[index:index+count];index+=count
                    if code==53:message=value[0]
                assert message in (1,3)
                response=bytearray(240);response[:4]=bytes([2,1,6,0]);response[4:8]=data[4:8]
                response[16:20]=self.guest;response[28:34]=data[28:34];response[236:240]=bytes([99,130,83,99])
                response+=bytes([53,1,2 if message==1 else 5,54,4])+self.gateway
                response+=bytes([1,4,255,255,255,0,3,4])+self.gateway+bytes([6,4])+self.dns+bytes([255])
                self.udp(bytes([255])*6,self.gateway,bytes([255])*4,67,68,response)
            elif dst==53 and target==self.dns:
                name=self.question(data);self.queries.append((name,src))
                if name in ('timeout.test','block.test'):return
                response=self.answer(data,name=='tcp.test')
                if name=='wrong.test':
                    bad=bytearray(response);bad[0]^=1
                    self.udp(mac,self.dns,source,53,src,bad)
                    bad=bytearray(response);bad[13]^=1
                    self.udp(mac,self.dns,source,53,src,bad)
                self.udp(mac,self.dns,source,53,src,response)
        elif header[9]==6 and target==self.dns:
            src,dst,seq,ack,offset,flags=struct.unpack_from('!HHIIBB',payload)
            if dst!=53:return
            pseudo=source+target+bytes([0,6])+struct.pack('!H',len(payload));assert not checksum(pseudo+payload)
            data=payload[(offset>>4)*4:]
            if flags&4:self.tcp.pop(src,None);return
            if flags&2:
                state=dict(mac=mac,ip=source,port=src,send=700000+src,receive=seq+1,buffer=bytearray(),responded=False)
                self.tcp[src]=state;self.tcp_packet(state,0x12);return
            state=self.tcp.get(src)
            if state is None:return
            if seq==state['receive'] and data:
                state['buffer']+=data;state['receive']+=len(data)
            if data:self.tcp_packet(state,0x10)
            buf=state['buffer']
            if len(buf)>=2 and len(buf)==2+struct.unpack_from('!H',buf)[0] and not state['responded']:
                state['responded']=True;response=self.answer(bytes(buf[2:]));packet=struct.pack('!H',len(response))+response
                # Delayed prefix pieces require actual native TCP reassembly and
                # partial length/body reads rather than one fabricated result.
                for piece in (packet[:1],packet[1:2],packet[2:9],packet[9:]):
                    self.tcp_packet(state,0x18,piece);time.sleep(.03)
            if flags&1 and seq+len(data)==state['receive']:
                state['receive']+=1;self.tcp_packet(state,0x11)

    def read(self,count):
        data=bytearray()
        while len(data)<count and not self.stop.is_set():
            try:part=self.connection.recv(count-len(data))
            except socket.timeout:continue
            if not part:raise EOFError('QEMU socket peer closed')
            data+=part
        return bytes(data)

    def run(self):
        try:
            while not self.stop.is_set():
                try:self.connection,_=self.listener.accept();break
                except socket.timeout:continue
            if self.stop.is_set():return
            self.connection.settimeout(.2)
            while not self.stop.is_set():
                prefix=self.read(4)
                if self.stop.is_set():break
                size=struct.unpack('!I',prefix)[0];assert 14<=size<=1514
                self.handle(self.read(size))
        except (OSError,EOFError) as error:
            if not self.stop.is_set():self.errors.append(repr(error))
        except BaseException as error:self.errors.append(repr(error))

    def close(self):
        self.stop.set()
        if self.connection:
            try:self.connection.shutdown(socket.SHUT_RDWR)
            except OSError:pass
            self.connection.close()
        self.listener.close();self.thread.join(timeout=3)
        assert not self.thread.is_alive() and not self.errors,self.errors
