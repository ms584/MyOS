"""Native Ring 3 simultaneous TCP streams; host only provides fixture peers."""
import socket
import subprocess
import threading
import time
from test_system import VM, OUT
from test_shell import command
from test_elf_dynamic import long_command, cancel_live, assert_live_resources
from make_asset_disk import create

def main():
    output=OUT/'elf-tcp';output.mkdir(exist_ok=True)
    listeners=[];errors=[];threads=[];barrier=threading.Barrier(2,timeout=15)
    def listener():
        sock=socket.socket();sock.bind(('127.0.0.1',0));sock.listen(8);sock.settimeout(40)
        listeners.append(sock);return sock
    a,b,c=listener(),listener(),listener()
    refused=socket.socket();refused.bind(('127.0.0.1',0));refused_port=refused.getsockname()[1];refused.close()
    def exact(peer,count):
        data=bytearray()
        while len(data)<count:
            part=peer.recv(count-len(data));assert part,'unexpected EOF';data.extend(part)
        return data
    def transfer(sock,side):
        try:
            for _ in range(2):
                peer,_=sock.accept()
                with peer:
                    peer.settimeout(20);assert exact(peer,1)==bytes([65+side]);barrier.wait()
                    time.sleep(.15);peer.sendall(bytes([97+side]))
                    payload=exact(peer,32768)
                    assert payload==bytes((i+side*73)%251 for i in range(32768))
                    assert peer.recv(1)==b'', 'shutdown must follow all queued bytes'
                    peer.sendall(bytes((i+side*19)%253 for i in range(20000)))
                    peer.shutdown(socket.SHUT_WR)
        except BaseException as error:errors.append(error)
    def cancellation(sock):
        try:
            for _ in range(2):
                peer,_=sock.accept()
                with peer:
                    peer.settimeout(15);assert exact(peer,1)==b'Q';peer.sendall(b'q')
            # Forced process teardown with a worker blocked in recv, then repeat.
            for _ in range(2):
                peer,_=sock.accept()
                with peer:
                    peer.settimeout(15)
                    try:assert peer.recv(1)==b''
                    except ConnectionResetError:pass
        except BaseException as error:errors.append(error)
    specs=[]
    for mode,name in ((None,'tcp'),('TCP_CANCEL','tcp-cancel'),('TCP_NIC_ABSENT','tcp-no-nic')):
        obj=output/(name+'.o');image=output/(name+'.so')
        arguments=['gcc','-m64','-std=c11','-Os','-fPIC','-ffreestanding','-fno-builtin',
                   '-fno-stack-protector','-fno-asynchronous-unwind-tables','-fno-unwind-tables',
                   '-Wall','-Wextra','-Werror','-Wno-unused-function',
                   f'-DPORT_A={a.getsockname()[1]}',f'-DPORT_B={b.getsockname()[1]}',
                   f'-DPORT_C={c.getsockname()[1]}',f'-DPORT_REFUSED={refused_port}']
        if mode:arguments.append(f'-D{mode}=1')
        subprocess.run([*arguments,'-c','user/elf-platform-tcp.c','-o',str(obj)],check=True)
        subprocess.run(['ld','-shared','--hash-style=both','-z','now','-z','relro','-e','_start',
                        '-o',str(image),str(obj)],check=True);specs.append(f'{name}={image}')
    disk=output/'assets.img';disk.unlink(missing_ok=True);create(disk,specs)
    bare=VM('elf-tcp-no-nic',asset_disk=disk)
    try:
        bare.wait('myos> ',25);result=long_command(bare,'elf live tcp-no-nic',timeout=10)
        assert 'ELF exited with status 45' in result,result;assert_live_resources(result)
    finally:bare.close()
    vm=VM('elf-tcp',nic=True,asset_disk=disk)
    try:
        vm.wait('myos> ',25);command(vm,'net up','Network: DHCP configured')
        # Start accept deadlines only after both VMs boot. The full regression
        # enables process self-tests, making boot longer than the scoped image.
        for target,args in ((transfer,(a,0)),(transfer,(b,1)),(cancellation,(c,))):
            thread=threading.Thread(target=target,args=args,daemon=True);thread.start();threads.append(thread)
        for _ in range(2):
            result=long_command(vm,'elf live tcp',timeout=50)
            assert 'TCP concurrent streams/readiness/EOF/cancel PASS' in result,result
            assert 'ELF exited with status 44' in result,result;assert_live_resources(result)
        for _ in range(2):cancel_live(vm,'tcp-cancel','TCP blocked worker ready',timeout=15)
        for thread in threads:thread.join(timeout=5);assert not thread.is_alive(), 'fixture server not complete'
        assert not errors,errors
        print('PASS: native concurrent TCP, partial I/O, epoll/poll, peek/FIONREAD, shutdown/EOF, refusal, dup/close/reuse and process cancellation')
    finally:
        vm.close()
        for sock in listeners:sock.close()

if __name__=='__main__':main()
