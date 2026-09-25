"""Offline Phase 29 HTTPS GUI navigation and fail-closed certificate test."""
import socket,ssl,subprocess,sys,threading
from pathlib import Path
sys.path.insert(0,'tests')
from test_system import VM,build,symbols
from test_shell import type_text
from test_browser import type_url,focus_address
from test_desktop import move_to,button,wait_count

CERTS=Path('security/certs')
def server(cert,connections):
    listener=socket.socket()
    listener.bind(('127.0.0.1',0));listener.listen(3);listener.settimeout(25)
    port=listener.getsockname()[1]
    seen=[];errors=[]
    def worker():
        ctx=ssl.SSLContext(ssl.PROTOCOL_TLS_SERVER)
        ctx.minimum_version=ssl.TLSVersion.TLSv1_2
        ctx.maximum_version=ssl.TLSVersion.TLSv1_2
        ctx.set_ciphers('ECDHE-ECDSA-AES128-GCM-SHA256')
        ctx.load_cert_chain(CERTS/cert,CERTS/'test-server.key')
        try:
            for _ in range(connections):
                raw,_=listener.accept();raw.settimeout(15)
                try:
                    with ctx.wrap_socket(raw,server_side=True) as conn:
                        request=conn.recv(1024)
                        seen.append(request)
                        path=request.split(b' ')[1]
                        if path==b'/next':
                            body=b'<html><head><title>Secure Next</title></head><body>Verified next page</body></html>'
                        else:
                            body=b'<html><head><title>Secure Start</title></head><body><a href="/next">Secure Next</a></body></html>'
                        conn.sendall(b'HTTP/1.0 200 OK\r\nContent-Type: text/html\r\nContent-Length: '+
                                     str(len(body)).encode()+b'\r\n\r\n'+body)
                except ssl.SSLError as error:errors.append(error)
        finally:listener.close()
    thread=threading.Thread(target=worker,daemon=True);thread.start()
    return port,thread,seen,errors

def main():
    try:
        build(28)
        vm=VM('phase29-browser-https',ram=16,nic=True,cpu='max')
        try:
            vm.wait('myos> ',30)
            type_text(vm,'desktop\n');vm.wait('DESKTOP READY')
            vm.key('7');vm.wait('WINDOW OPEN 7')
            addr=symbols()
            port,thread,seen,errors=server('test-server.pem',2)
            type_url(vm,f'https://10.0.2.2:{port}/')
            vm.wait('BROWSER: Secure Start',25)
            data=vm.memory(addr['lines'],160*(68+192+1))
            assert b'Secure Next' in data and b'/next' in data
            move_to(vm,addr,245,287);button(vm,True);button(vm,False)
            vm.wait('BROWSER: Secure Next',20)
            move_to(vm,addr,225,200);button(vm,True)
            wait_count(vm,'BROWSER: Secure Start',2)
            button(vm,False)
            assert len(seen)==2,seen
            bad_port,bad_thread,bad_seen,bad_errors=server('wrong-host.pem',1)
            focus_address(vm,addr)
            type_url(vm,f'https://10.0.2.2:{bad_port}/')
            vm.wait('HTTPS failed, cert rejected or too large',20)
            assert vm.log.read_text().count('BROWSER: Secure Start')>=2
            assert b'Secure Next' in vm.memory(addr['lines'],160*(68+192+1))
            vm.key('esc');vm.wait('DESKTOP CLOSED\nmyos> ',8)
            print('PASS: 16 MiB HTTPS GUI navigation, link, Back and wrong-host rejection',flush=True)
        finally:vm.close()
    finally:build()

if __name__=='__main__':main()
