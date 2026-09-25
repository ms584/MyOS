"""Phase 27 RTL8139 smoke test against the packaged UEFI image."""
from pathlib import Path
from test_network import send
from test_system import VM

def main():
    code=Path('/usr/share/OVMF/OVMF_CODE_4M.fd')
    vars=Path('/usr/share/OVMF/OVMF_VARS_4M.fd')
    vm=VM('phase27-uefi',ram=128,
          boot_image=Path('build/myos-uefi.img'),boot_bus='ide',
          firmware=(code,vars),nic=True)
    try:
        vm.wait('myos> ',35)
        send(vm,'net up')
        vm.wait('IP 10.0.2.15 gateway 10.0.2.2 DNS 10.0.2.3',15)
        send(vm,'net dns localhost')
        vm.wait('DNS localhost = 127.0.0.1',15)
        print('PASS: packaged UEFI image, RTL8139, DHCP and DNS')
    finally:vm.close()

if __name__=='__main__':main()
