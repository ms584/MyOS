"""Phase 28 native TLS acceptance and rejection tests. Run from repo root."""
import socket,ssl,sys,threading,time
from pathlib import Path
sys.path.insert(0,'tests')
from test_system import VM,build
from test_network import send

CERTS=Path('security/certs')

def server(cert='test-server.pem'):
    listener=socket.socket();listener.setsockopt(socket.SOL_SOCKET,socket.SO_REUSEADDR,1)
    listener.bind(('127.0.0.1',0));listener.listen(1);listener.settimeout(35)
    port=listener.getsockname()[1]
    failures=[]
    def worker():
        try:
            ctx=ssl.SSLContext(ssl.PROTOCOL_TLS_SERVER)
            ctx.minimum_version=ssl.TLSVersion.TLSv1_2
            ctx.maximum_version=ssl.TLSVersion.TLSv1_2
            rsa=cert.startswith('rsa-')
            ctx.set_ciphers('ECDHE-RSA-AES128-GCM-SHA256' if rsa else 'ECDHE-ECDSA-AES128-GCM-SHA256')
            ctx.load_cert_chain(CERTS/cert,CERTS/('rsa-server.key' if rsa else 'test-server.key'))
            raw,_=listener.accept();raw.settimeout(20)
            with ctx.wrap_socket(raw,server_side=True) as conn:
                request=conn.recv(1024)
                if b'GET / HTTP/1.0' not in request:raise AssertionError(request)
                conn.sendall(b'HTTP/1.0 200 OK\r\nContent-Length: 10\r\n\r\nTLS_OK_28!')
        except Exception as exc:failures.append(exc)
        finally:listener.close()
    thread=threading.Thread(target=worker,daemon=True);thread.start()
    return port,thread,failures

def attempt(name,cert,expected,cpu='max',rtc=None,connect=True):
    vm=VM(name,nic=True,cpu=cpu,rtc=rtc)
    try:
        vm.wait('myos> ',30)
        send(vm,'net up');vm.wait('IP 10.0.2.15',15)
        if connect:port,thread,failures=server(cert)
        else:port=443
        send(vm,f'net https 10.0.2.2 {port} /')
        content=vm.wait(expected,45)
        if expected=='HTTPS verified':
            assert 'TLS_OK_28!' in content,content
            thread.join(3);assert not failures,failures
        else:assert 'HTTPS verified' not in content,content
        print('PASS: '+name,flush=True)
    finally:vm.close()

def main():
    try:
        build(28)
        attempt('phase28-valid','test-server.pem','HTTPS verified')
        attempt('phase28-valid-rsa','rsa-server.pem','HTTPS verified')
        attempt('phase28-wrong-host','wrong-host.pem','HTTPS request failed or certificate rejected')
        attempt('phase28-untrusted','untrusted-server.pem','HTTPS request failed or certificate rejected')
        attempt('phase28-expired','test-server.pem','HTTPS request failed or certificate rejected',rtc='2040-01-01')
        attempt('phase28-bad-clock','test-server.pem','HTTPS request failed or certificate rejected',rtc='2010-01-01',connect=False)
        attempt('phase28-no-rng','test-server.pem','HTTPS request failed or certificate rejected',cpu='qemu64',connect=False)
        build()
        attempt('phase28-production-trust','test-server.pem','HTTPS request failed or certificate rejected')
    finally:build()

if __name__=='__main__':main()
