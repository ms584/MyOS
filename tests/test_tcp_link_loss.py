"""Native idle TCP carrier loss. Host supplies bytes only, never media or TLS."""
from pathlib import Path
import argparse, json, re, shutil, socket, subprocess, threading, time
from test_system import ROOT, OUT, VM
from test_shell import command, type_text
from test_elf_dynamic import assert_live_resources
from make_asset_disk import create

def main(argv=None):
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--uefi',action='store_true')
    parser.add_argument('--expect-missing',action='store_true',help='Reproduce the old idle carrier-loss bug only')
    args=parser.parse_args(argv)
    out=OUT/('tcp-link-loss-uefi' if args.uefi else 'tcp-link-loss-bios')
    out.mkdir(exist_ok=True)
    server=socket.socket();server.bind(('127.0.0.1',0));server.listen(2);server.settimeout(30)
    errors=[];release=threading.Event()
    def fixture():
        try:
            for expected in ([b'A'] if args.expect_missing else [b'A',b'R']):
                peer,_=server.accept()
                with peer:
                    peer.settimeout(20)
                    assert peer.recv(1)==expected
                    peer.sendall(expected.lower())
                    if expected==b'A': assert release.wait(20), 'loss notification not observed'
                    else:
                        try: assert peer.recv(1)==b''
                        except ConnectionResetError: pass
        except BaseException as error: errors.append(repr(error))
    image=out/'fixture.so';obj=out/'fixture.o'
    subprocess.run(['gcc','-m64','-std=c11','-Os','-fPIC','-ffreestanding','-fno-builtin',
        '-fno-stack-protector','-fno-asynchronous-unwind-tables','-fno-unwind-tables',
        '-Wall','-Wextra','-Werror','-Wno-unused-function','-DTCP_LINK_LOSS=1',
        '-DPORT_C='+str(server.getsockname()[1]),'-c','user/elf-platform-tcp.c','-o',str(obj)],check=True)
    subprocess.run(['ld','-shared','--hash-style=both','-z','now','-z','relro','-e','_start','-o',str(image),str(obj)],check=True)
    disk=out/'assets.img';disk.unlink(missing_ok=True);create(disk,['loss='+str(image)])
    options={}
    if args.uefi:
        options=dict(ram=128,boot_image=ROOT/'build/myos-uefi.img',boot_bus='ide',firmware=(Path('/usr/share/OVMF/OVMF_CODE_4M.fd'),Path('/usr/share/OVMF/OVMF_VARS_4M.fd')))
    vm=VM('tcp-link-loss-uefi' if args.uefi else 'tcp-link-loss',nic=True,asset_disk=disk,**options)
    thread=threading.Thread(target=fixture,daemon=True)
    result=dict(baseline_reproduction=args.expect_missing,native=True,passed=False)
    try:
        vm.wait('myos> ',40);command(vm,'net up','Network: DHCP configured')
        thread.start();offset=len(vm.log.read_text())
        type_text(vm,'elf live loss'+chr(10));vm.wait('TCP idle link-loss ready',20)
        network=vm.command('human-monitor-command',{'command-line':'info network'})
        names=re.findall(r'^([^ :\n]+):.*type=nic',network,re.M);assert len(names)==1
        start=time.monotonic();vm.command('set_link',{'name':names[0],'up':False})
        if args.expect_missing:
            vm.wait('ELF exited with status 93',12)
            result['missing_idle_error_reproduced']=True;release.set()
        else:
            vm.wait('TCP link recovery ready',10);result['loss_notification_seconds']=time.monotonic()-start
            assert result['loss_notification_seconds']<5;release.set()
            vm.command('set_link',{'name':names[0],'up':True})
            vm.wait('TCP idle carrier loss/blocked recv/poll/reconnect PASS',20)
            vm.wait('ELF exited with status 46',10);result['passed']=True
        vm.wait('myos> ',10)
        assert_live_resources(vm.log.read_text()[offset:])
        thread.join(5);assert not thread.is_alive() and not errors,errors
        print('BASELINE idle connection failed to report carrier loss' if args.expect_missing else 'PASS native idle carrier loss, blocked recv, poll, SO_ERROR, reconnect and exact cleanup')
    finally:
        release.set();shutil.copyfile(vm.log,out/'serial.log');vm.close();server.close()
        (out/'result.json').write_text(json.dumps(result,indent=2)+chr(10))
if __name__=='__main__':main()
