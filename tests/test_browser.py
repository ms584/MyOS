"""Phase 29 preliminary browser GUI and local HTTP navigation in QEMU."""
from http.server import BaseHTTPRequestHandler,ThreadingHTTPServer
from pathlib import Path
import sys,threading,time
sys.path.insert(0,'tests')
from test_system import VM,OUT,symbols
from test_shell import type_text
from test_desktop import move_to,button,shot,wait_count

class Pages(BaseHTTPRequestHandler):
    def do_GET(self):
        if self.path=='/redirect':
            self.send_response(302);self.send_header('Location','https://example.com/')
            self.send_header('Content-Length','0');self.end_headers();return
        if self.path=='/large':
            body=b'A'*40000
        elif self.path=='/next':
            body=b'<html><head><title>Second page</title></head><body><h1>Second page</h1><a href="/">Home</a></body></html>'
        else:
            body=b'<html><head><title>Phase 29 Test</title><style>HIDDEN STYLE</style></head><body><h1>Phase 29 Test</h1><p>Local page &amp; links</p><a href="/next">Next page</a><script>HIDDEN SCRIPT</script></body></html>'
        self.send_response(200);self.send_header('Content-Type','text/html');self.send_header('Content-Length',str(len(body)));self.end_headers()
        try:self.wfile.write(body)
        except BrokenPipeError:pass
    def log_message(self,*args):pass

def type_url(vm,url,submit=True):
    for c in url:
        if c==':':vm.key('shift','semicolon')
        else:vm.key({'/':'slash','.':'dot','-':'minus'}.get(c,c))
    if submit:vm.key('ret')

def focus_address(vm,addresses):
    frames=vm.log.read_text().count('DESKTOP FRAME')
    move_to(vm,addresses,300,200);button(vm,True);button(vm,False)
    wait_count(vm,'DESKTOP FRAME',frames+1)

def main():
    server=ThreadingHTTPServer(('127.0.0.1',0),Pages)
    threading.Thread(target=server.serve_forever,daemon=True).start()
    bare=VM('phase29-browser-no-nic')
    try:
        bare.wait('myos> ',25)
        type_text(bare,'desktop\n');bare.wait('DESKTOP READY')
        bare.key('7');bare.wait('WINDOW OPEN 7')
        type_url(bare,'http://10.0.2.2:8000/')
        bare.wait('Network unavailable; run net up',8)
        bare.key('esc');bare.wait('DESKTOP CLOSED\nmyos> ',8)
    finally:bare.close()
    vm=VM('phase29-browser',nic=True)
    try:
        vm.wait('myos> ',25)
        type_text(vm,'desktop\n');vm.wait('DESKTOP READY')
        vm.key('7');vm.wait('WINDOW OPEN 7')
        type_url(vm,f'http://10.0.2.2:{server.server_port}/',submit=False)
        frames=vm.log.read_text().count('DESKTOP FRAME')
        vm.key('ret')
        vm.wait('BROWSER: Phase 29 Test',20)
        wait_count(vm,'DESKTOP FRAME',frames+1)
        pixel=shot(vm,'phase29-browser.ppm')
        assert pixel(214,251)==(0xf5,0xf7,0xfa),pixel(214,251)
        addresses=symbols()
        parsed=vm.memory(addresses['lines'],160*(68+192+1))
        assert b'Phase 29 Test' in parsed and b'Next page' in parsed and b'Local page & links' in parsed,parsed[:300]
        assert b'HIDDEN STYLE' not in parsed and b'HIDDEN SCRIPT' not in parsed
        move_to(vm,addresses,245,323);button(vm,True)
        vm.wait('BROWSER: Second page',12);button(vm,False)
        move_to(vm,addresses,225,200);button(vm,True)
        wait_count(vm,'BROWSER: Phase 29 Test',2);button(vm,False)
        focus_address(vm,addresses)
        type_url(vm,f'http://10.0.2.2:{server.server_port}/large')
        vm.wait('HTTP failed, timed out or too large',20)
        focus_address(vm,addresses)
        type_url(vm,f'http://10.0.2.2:{server.server_port}/next')
        wait_count(vm,'BROWSER: Second page',2)
        focus_address(vm,addresses)
        type_url(vm,f'http://10.0.2.2:{server.server_port}/redirect')
        vm.wait('Redirect requires HTTPS (Phase 28)',8)
        # The UI makes unsupported online search and HTTPS explicit.
        focus_address(vm,addresses)
        type_url(vm,'cats')
        vm.wait('Online search requires HTTPS (Phase 28)',8)
        focus_address(vm,addresses)
        type_url(vm,'https://example.com/')
        vm.wait('HTTPS requires Phase 28',8)
        vm.key('esc');vm.wait('DESKTOP CLOSED\nmyos> ',8)
        assert 'PANIC' not in vm.log.read_text()
        print('PASS: browser GUI, HTML entities/links/Back, oversized response recovery, HTTPS redirect/search limits and close')
    finally:vm.close();server.shutdown();server.server_close()

def low_memory():
    server=ThreadingHTTPServer(('127.0.0.1',0),Pages)
    threading.Thread(target=server.serve_forever,daemon=True).start()
    vm=VM('phase29-browser-16m',ram=16,nic=True)
    try:
        vm.wait('myos> ',25)
        type_text(vm,'desktop\n');vm.wait('DESKTOP READY')
        vm.key('7');vm.wait('WINDOW OPEN 7')
        type_url(vm,f'http://10.0.2.2:{server.server_port}/')
        vm.wait('BROWSER: Phase 29 Test',20)
        vm.key('esc');vm.wait('DESKTOP CLOSED\nmyos> ',8)
        print('PASS: browser local HTTP page at 16 MiB RAM')
    finally:vm.close();server.shutdown();server.server_close()

if __name__=='__main__':main();low_memory()
