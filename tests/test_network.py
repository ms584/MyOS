"""Phase 27 QEMU RTL8139 protocol and recovery acceptance. Run from repo root."""
import socket
import sys
import threading
import time
sys.path.insert(0,'tests')
from test_system import VM

def send(vm,command):
    keys={' ':'spc','.':'dot','/':'slash','-':'minus'}
    for letter in command:vm.key(keys.get(letter,letter))
    vm.key('ret')

def echo_server(sock):
    try:
        data,peer=sock.recvfrom(2048)
        sock.sendto(data,peer)
    except OSError:pass

def http_server(sock,body):
    try:
        client,_=sock.accept();client.settimeout(5)
        request=client.recv(2048)
        assert b'GET / HTTP/1.0' in request,request
        client.sendall(b'HTTP/1.0 200 OK\r\nContent-Length: '+str(len(body)).encode()+b'\r\n\r\n'+body)
        client.close()
    except OSError:pass

def listen(body=b'NET_OK!\n'):
    sock=socket.socket();sock.bind(('127.0.0.1',0));sock.listen(1);sock.settimeout(20)
    threading.Thread(target=http_server,args=(sock,body),daemon=True).start()
    return sock

def wait_count(vm,text,count,timeout=15):
    end=time.monotonic()+timeout
    while time.monotonic()<end:
        content=vm.log.read_text()
        if content.count(text)>=count:return content
        if 'PANIC' in content:raise AssertionError(content)
        time.sleep(.05)
    raise AssertionError(f'missing {count} occurrences of {text!r}\n{vm.log.read_text()}')

def main():
    bare=VM('phase27-no-nic')
    try:
        bare.wait('myos> ',25)
        send(bare,'net status');bare.wait('Network: no RTL8139 NIC',5)
        send(bare,'net up');bare.wait('Network: DHCP failed or timed out',5)
    finally:bare.close()
    vm=VM('phase27-network',nic=True)
    sockets=[]
    try:
        vm.wait('myos> ',25)
        send(vm,'net status');vm.wait('Network: RTL8139, no DHCP lease')
        send(vm,'net up');vm.wait('IP 10.0.2.15 gateway 10.0.2.2 DNS 10.0.2.3',12)
        send(vm,'net dns localhost');vm.wait('DNS localhost = 127.0.0.1',12)
        udp=socket.socket(socket.AF_INET,socket.SOCK_DGRAM)
        udp.bind(('127.0.0.1',0));udp.settimeout(20);sockets.append(udp)
        threading.Thread(target=echo_server,args=(udp,),daemon=True).start()
        send(vm,f'net udp 10.0.2.2 {udp.getsockname()[1]} hello')
        vm.wait('UDP reply: hello',12)
        http=listen();sockets.append(http)
        send(vm,f'net get 10.0.2.2 {http.getsockname()[1]} /')
        vm.wait('NET_OK!',15)
        # A refused TCP port must not poison the next connection.
        closed=socket.socket();closed.bind(('127.0.0.1',0));port=closed.getsockname()[1];closed.close()
        send(vm,f'net get 10.0.2.2 {port} /')
        vm.wait('Network request failed or timed out',12)
        http2=listen();sockets.append(http2)
        send(vm,f'net get 10.0.2.2 {http2.getsockname()[1]} /')
        wait_count(vm,'NET_OK!',2,15)
        # UDP timeout/retry followed by a successful exchange.
        closed=socket.socket(socket.AF_INET,socket.SOCK_DGRAM)
        closed.bind(('127.0.0.1',0));port=closed.getsockname()[1];closed.close()
        send(vm,f'net udp 10.0.2.2 {port} lost')
        wait_count(vm,'Network request failed or timed out',2,12)
        udp2=socket.socket(socket.AF_INET,socket.SOCK_DGRAM)
        udp2.bind(('127.0.0.1',0));udp2.settimeout(20);sockets.append(udp2)
        threading.Thread(target=echo_server,args=(udp2,),daemon=True).start()
        send(vm,f'net udp 10.0.2.2 {udp2.getsockname()[1]} again')
        vm.wait('UDP reply: again',12)
        large=listen(b'A'*4096+b'TAIL_OK\n');sockets.append(large)
        send(vm,f'net get 10.0.2.2 {large.getsockname()[1]} /')
        vm.wait('TAIL_OK',20)
        send(vm,'net up');wait_count(vm,'Network: DHCP configured',2,12)
        print('PASS: RTL8139, DHCP, ARP, DNS, UDP, TCP/HTTP streaming, failure, timeout and reconnect')
    finally:
        vm.close()
        for sock in sockets:sock.close()

if __name__=='__main__':main()
