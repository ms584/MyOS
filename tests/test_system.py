"""End-to-end x86-64 QEMU tests. Run from the MyOS repo root.

Restores the normal framebuffer build even after a failing test. Generated
images, logs and screenshots live in build/system-tests. The normal suite needs no data disk; storage has its own isolated test.
"""
import argparse
import json
import os
from pathlib import Path
import socket
import subprocess
import time
import sys
import shlex

def trace_command(event,args):
    if event=="subprocess.Popen":
        print("$ "+shlex.join(args[1]),flush=True)
if os.environ.get("MYOS_TRACE")=="1" and os.environ.get("MYOS_TRACE_HOOK")!="1":
    sys.addaudithook(trace_command)
    os.environ["MYOS_TRACE_HOOK"]="1"

ROOT=Path.cwd()
OUT=ROOT/'build/system-tests'
OUT.mkdir(parents=True,exist_ok=True)

def build(case=0,text=False):
    env=dict(os.environ,TEST_CASE=str(case),TEXT_ONLY=str(int(text)),PROCESS_TESTS="1")
    subprocess.run(['bash','build.sh'],env=env,check=True)

class VM:
    def __init__(self,name,ram=64,disk=None):
        self.name=name
        self.log=OUT/(name+'.log')
        self.sock=OUT/(name+'.sock')
        self.log.write_text('')
        self.sock.unlink(missing_ok=True)
        command=['qemu-system-x86_64','-m',str(ram),'-display','none',
            '-monitor','none','-serial',f'file:{self.log}','-no-reboot','-no-shutdown',
            '-drive',f'format=raw,file={ROOT}/build/myos-current.img,if=floppy',
            '-boot','order=a','-qmp',f'unix:{self.sock},server=on,wait=off']
        if disk is not None:command.extend(['-drive',f'format=raw,file={disk},if=ide,index=0'])
        self.process=subprocess.Popen(command)
        self.connection=socket.socket(socket.AF_UNIX)
        try:
            for _ in range(100):
                try:self.connection.connect(str(self.sock));break
                except (FileNotFoundError,ConnectionRefusedError):time.sleep(.05)
            self.connection.settimeout(5)
            self.stream=self.connection.makefile('rwb')
            json.loads(self.stream.readline())
            self.command('qmp_capabilities')
        except BaseException:
            self.process.terminate();self.process.wait(timeout=5);raise
    def command(self,name,arguments=None):
        message={'execute':name}
        if arguments is not None:message['arguments']=arguments
        self.stream.write(json.dumps(message).encode()+b'\n');self.stream.flush()
        while True:
            line=self.stream.readline()
            if not line:raise RuntimeError('QMP disconnected')
            reply=json.loads(line)
            if 'error' in reply:raise RuntimeError(reply)
            if 'return' in reply:return reply['return']
    def wait(self,text,timeout=15):
        end=time.monotonic()+timeout
        while time.monotonic()<end:
            content=self.log.read_text()
            if text in content:return content
            if 'PANIC' in content and 'PANIC' not in text:raise AssertionError(content)
            time.sleep(.05)
        raise AssertionError(f'{self.name}: missing {text!r}\n{self.log.read_text()}')
    def memory(self,address,size):
        dump=OUT/(self.name+'.memory')
        result=self.command('human-monitor-command',{'command-line':f'pmemsave {address:#x} {size} "{dump}"'})
        assert not result, result
        return dump.read_bytes()
    def key(self,*codes):
        def event(code,down):return {'type':'key','data':{'down':down,'key':{'type':'qcode','data':code}}}
        self.command('input-send-event',{'events':[event(c,True) for c in codes]})
        time.sleep(.04)
        self.command('input-send-event',{'events':[event(c,False) for c in reversed(codes)]})
        time.sleep(.04)
    def close(self):
        self.process.terminate();self.process.wait(timeout=5)
        self.stream.close();self.connection.close();self.sock.unlink(missing_ok=True)

def symbols():
    result={}
    for line in subprocess.check_output(['nm','build/kernel.elf'],text=True).splitlines():
        fields=line.split()
        if len(fields)==3:result[fields[2]]=int(fields[0],16)
    return result

def boot_test(name,ram=64,keyboard=False,text=False):
    vm=VM(name,ram)
    try:
        content=vm.wait('myos> ')
        for marker in ('64-bit Long Mode','PASS: PMM, paging, heap, exhaustion/recovery',
                       'PASS: two CPU-bound tasks preempted; task exit',
                       'PASS: context registers and direction flag preserved',
                       'PASS: Ring 3 processes, private CR3/data/stack, timer preemption',
                       'PASS: 13 user fault cases contained; kernel and peer survived',
                       'PASS: process kill/reap, capacity, PID and resource reuse',
                       'PASS: syscall ABI, 29 user checks, pointer validation, signed exit',
                       'PASS: user write/getpid/yield/exit; status=42, resources reclaimed',
                       'PASS: RAMFS paths, binary I/O, offsets, permissions and cleanup',
                       'PASS: file syscalls, safe copyout, exit/fault/kill descriptor cleanup',
                       'Welcome to MyOS RAMFS.',
                       'PASS: Ring 3 opened/read/closed /etc/motd'):
            assert marker in content,content
        assert 'LEAK' not in content, 'Invalid write produced partial output'
        assert 'SYS_TEST_RX\nABCDOK\n' in content, content
        assert ('VGA text' if text else 'framebuffer 800x600x32') in content
        syms=symbols()
        first=int.from_bytes(vm.memory(syms['ticks'],8),'little')
        counters=[int.from_bytes(vm.memory(syms['task_counts']+i*8,8),'little') for i in range(2)]
        time.sleep(.4)
        last=int.from_bytes(vm.memory(syms['ticks'],8),'little')
        assert 10 <= last-first <= 200,(first,last)
        assert all(int.from_bytes(vm.memory(syms['task_counts']+i*8,8),'little')>counters[i] for i in range(2))
        if keyboard:
            vm.key('a');vm.key('shift','b');vm.key('caps_lock');vm.key('c');vm.key('shift','d')
            vm.key('1');vm.key('shift','1');vm.key('backspace');vm.key('ret')
            vm.wait('error: unknown command: aBCd1\nmyos> ')
            vm.key('caps_lock');vm.key('up');vm.key('e')
            vm.wait('\nmyos> e')
            # Extended keys are ignored, releases do not echo and backspace erases.
            raw=vm.memory(syms['cells'],4000)
            width=80 if text else 100
            assert b'myos> aBCd1 ' in raw and b'myos> e' in raw
            # Force wrapping and scrolling, then check the final logical screen.
            for _ in range(42):vm.key('ret')
            time.sleep(.2)
            raw=vm.memory(syms['cells'],4000)
            height=25 if text else 37
            assert raw[(height-1)*width:(height-1)*width+6]==b'myos> '
        screenshot=OUT/(name+'.ppm')
        vm.command('screendump',{'filename':str(screenshot)})
        data=screenshot.read_bytes();assert data.startswith(b'P6\n')
        if not text:
            assert len(data)>800*600*3
            assert any(data[-800*600*3:]),'Framebuffer is entirely black'
        print(f'PASS: {name}: boot/memory/preemption/timer'+('/keyboard/console-scroll' if keyboard else ''),flush=True)
    finally:vm.close()

def fault_test(case,marker,error):
    build(case)
    vm=VM(f'fault-{case}')
    try:
        content=vm.wait('PANIC: '+marker)
        assert f'error=0x{error:016X}' in content,content
        if case in (2,3,4):assert 'cr2=0x0000000200000000' in content,content
        else:assert f"rip=0x{symbols()['fault_site']:016X}" in content,content
        print(f'PASS: {marker}, error={error:#x}',flush=True)
    finally:vm.close()

def main():
    parser=argparse.ArgumentParser();parser.add_argument('--suite',choices=['all','boot','memory','keyboard','exceptions','timer','processes','syscalls','filesystem','shell','storage'],default='all');args=parser.parse_args()
    try:
        build()
        boot_test('framebuffer-64M',keyboard=args.suite in ('all','keyboard'))
        if args.suite in ('all','shell'):
            from test_shell import exercise
            exercise()
        if args.suite in ('all','storage'):
            from test_storage import exercise
            exercise()
        if args.suite in ('all','memory'):
            for ram in (16,256):boot_test(f'ram-{ram}M',ram)
        if args.suite in ('all','exceptions'):
            fault_test(1,'Invalid Opcode',0)
            fault_test(6,'General Protection Fault',0x48)
            fault_test(2,'Page Fault',2)
            fault_test(3,'Page Fault',3)
            fault_test(4,'Page Fault',17)
        if args.suite=='all':
            build(text=True);boot_test('vga-fallback',keyboard=True,text=True)
            from test_shell import exercise
            exercise(text=True)
    finally:
        env=dict(os.environ,TEST_CASE="0",TEXT_ONLY="0",PROCESS_TESTS="0")
        subprocess.run(["bash","build.sh"],env=env,check=True)
    print('PASS: selected suite; normal framebuffer image restored',flush=True)

if __name__=='__main__':main()
