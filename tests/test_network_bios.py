"""Phase 27 NIC and Phase 26 v2 data-disk smoke test in packaged BIOS boot."""
from pathlib import Path
import subprocess
from test_network import send
from test_system import VM,OUT

def main():
    disk=OUT/'phase27-v2-data.img'
    disk.unlink(missing_ok=True)  # This test owns this generated fixture.
    subprocess.run(['python3','tools/create_data_disk.py','--large',str(disk)],check=True)
    vm=VM('phase27-bios',ram=64,boot_image=Path('build/myos-bios.img'),
          boot_bus='ide',disk=disk,disk_index=1,nic=True)
    try:
        content=vm.wait('myos> ',35)
        assert 'Storage: empty data disk; first change will save' in content,content
        send(vm,'disk');vm.wait('Disk: ATA PIO',6)
        send(vm,'net up');vm.wait('IP 10.0.2.15 gateway 10.0.2.2 DNS 10.0.2.3',15)
        print('PASS: packaged BIOS image, v2 data slave, RTL8139 and DHCP')
    finally:vm.close()

if __name__=='__main__':main()
