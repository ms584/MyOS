"""Actual Ring 3 timeval deadlines and shared socket-option ownership."""
import socket
import re
import subprocess
import threading
import time
from test_system import VM,OUT
from test_shell import command,type_text
from test_elf_dynamic import assert_live_resources
from make_asset_disk import create

def main():
    out=OUT/'socket-options';out.mkdir(exist_ok=True)
    listener=socket.socket();listener.setsockopt(socket.SOL_SOCKET,socket.SO_RCVBUF,1024)
    listener.bind(('127.0.0.1',0));listener.listen(4);listener.settimeout(40)
    refused=socket.socket();refused.bind(('127.0.0.1',0));refused_port=refused.getsockname()[1];refused.close()
    errors=[]
    def peer():
        try:
            for _ in range(2):
                connection,_=listener.accept()
                with connection:
                    connection.settimeout(10);connection.sendall(b's')
                    data=bytearray()
                    while True:
                        part=connection.recv(4096)
                        if not part:break
                        data.extend(part)
                    assert len(data)>=4096 and all(value==ord('Z') for value in data),data[-32:]
                    connection.sendall(b'rd');connection.shutdown(socket.SHUT_WR)
        except BaseException as error:errors.append(error)
    obj=out/'options.o';image=out/'options.so';disk=out/'assets.img'
    subprocess.run(['gcc','-m64','-std=c11','-Os','-fPIC','-ffreestanding','-fno-builtin',
                    '-fno-stack-protector','-fno-asynchronous-unwind-tables','-fno-unwind-tables',
                    '-Wall','-Wextra','-Werror',f'-DPORT={listener.getsockname()[1]}',f'-DREFUSED={refused_port}',
                    '-c','user/elf-platform-socket-options.c','-o',str(obj)],check=True)
    subprocess.run(['ld','-shared','--hash-style=both','-z','now','-z','relro','-e','_start',
                    '-o',str(image),str(obj)],check=True)
    disk.unlink(missing_ok=True);create(disk,[f'options={image}'])
    vm=VM('socket-options',nic=True,asset_disk=disk)
    try:
        vm.wait('myos> ',30);command(vm,'net up','Network: DHCP configured')
        network=vm.command('human-monitor-command',{'command-line':'info network'})
        match=re.search(r'(\S+): index=\d+,type=user,',network);assert match,network
        netdev=match.group(1)
        thread=threading.Thread(target=peer,daemon=True);thread.start()
        def wait_marker(offset,marker,timeout=20):
            end=time.monotonic()+timeout
            while time.monotonic()<end:
                output=vm.log.read_text()[offset:];assert 'PANIC' not in output,output
                if marker in output:return output
                assert not output.endswith('myos> '),output
                time.sleep(.01)
            raise AssertionError((marker,vm.log.read_text()[offset:]))
        for _ in range(2):
            offset=len(vm.log.read_text());type_text(vm,'elf live options\n')
            for phase in ('connect','send'):
                wait_marker(offset,f'TCP {phase} stall ready\n')
                vm.command('stop')
                try:
                    # Real packet delay; never alter guest memory or syscall results.
                    vm.command('object-add',{'qom-type':'filter-buffer','id':'socket-options-stall',
                                            'netdev':netdev,'interval':5000000})
                finally:vm.command('cont')
                wait_marker(offset,f'TCP {phase} timeout checked\n')
                vm.command('object-del',{'id':'socket-options-stall'})
            output=wait_marker(offset,'myos> ')
            assert 'Socket timeval/deadline/dup/backpressure/cancel PASS' in output,output
            assert 'ELF exited with status 46' in output,output;assert_live_resources(output)
        thread.join(timeout=5);assert not thread.is_alive(),'fixture peer unfinished';assert not errors,errors
        print('PASS: native socket timeval ABI, endpoint/dup options, finite/zero/immediate/overflow deadlines, '
              'nonblocking override, partial I/O, actual TCP connect/send stalls and recovery, '
              'repeated wakes and close/reuse cancellation')
    finally:vm.close();listener.close()

if __name__=='__main__':main()
