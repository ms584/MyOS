"""Browser smoke test in packaged BIOS and UEFI MyOS images."""
from http.server import ThreadingHTTPServer
from pathlib import Path
import sys,threading
sys.path.insert(0,'tests')
from test_browser import Pages,type_url
from test_system import VM
from test_shell import type_text

def main():
    server=ThreadingHTTPServer(('127.0.0.1',0),Pages)
    threading.Thread(target=server.serve_forever,daemon=True).start()
    try:
        for firmware in ('bios','uefi'):
            fw=None
            if firmware=='uefi':fw=(Path('/usr/share/OVMF/OVMF_CODE_4M.fd'),Path('/usr/share/OVMF/OVMF_VARS_4M.fd'))
            vm=VM('phase29-'+firmware,ram=128 if fw else 64,
                  boot_image=Path('build/myos-'+firmware+'.img'),boot_bus='ide',firmware=fw,nic=True)
            try:
                vm.wait('myos> ',40)
                type_text(vm,'desktop\n');vm.wait('DESKTOP READY')
                vm.key('7');vm.wait('WINDOW OPEN 7')
                type_url(vm,f'http://10.0.2.2:{server.server_port}/')
                vm.wait('BROWSER: Phase 29 Test',25)
                vm.key('esc');vm.wait('DESKTOP CLOSED\nmyos> ',10)
                print(f'PASS: packaged {firmware.upper()} browser local HTTP page',flush=True)
            finally:vm.close()
    finally:server.shutdown();server.server_close()

if __name__=='__main__':main()
