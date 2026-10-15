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
    def __init__(self,name,ram=64,disk=None,boot_image=None,boot_bus='floppy',disk_index=0,firmware=None,usb=False,nic=False,audio=None,cpu=None,rtc=None,asset_disk=None,xhci_devices=None,usb_disk=None,usb_sector_size=512,xhci_usb3=True):
        self.name=name
        self.log=OUT/(name+'.log')
        self.sock=OUT/(name+'.sock')
        self.log.write_text('')
        self.sock.unlink(missing_ok=True)
        if boot_bus not in ('floppy','ide'):
            raise ValueError('boot_bus must be floppy or ide')
        image=boot_image or ROOT/'build/myos-current.img'
        command=['qemu-system-x86_64','-m',str(ram),'-display','none',
            '-monitor','none','-serial',f'file:{self.log}','-no-reboot','-no-shutdown',
            '-drive',f'format=raw,file={image},if={boot_bus},index=0',
            '-boot','order='+('c' if boot_bus=='ide' else 'a'),
            '-qmp',f'unix:{self.sock},server=on,wait=off']
        if xhci_devices is not None:
            command.extend(['-device','qemu-xhci,id=xhci'+('' if xhci_usb3 else ',p3=0')])
            for device in xhci_devices:command.extend(['-device',device])
        if usb:
            index=command.index('-drive')
            command[index+1]=f'format=raw,file={image},if=none,id=bootusb'
            command.extend(['-usb','-device','usb-storage,drive=bootusb,bootindex=1'+(',bus=xhci.0' if xhci_devices is not None else '')])
        if usb_disk is not None:
            if xhci_devices is None or usb_sector_size not in (512,4096):raise ValueError('invalid USB test fixture')
            command.extend(['-drive',f'format=raw,file={usb_disk},if=none,id=testusb',
                            '-device',f'usb-storage,drive=testusb,bus=xhci.0,id=teststorage,logical_block_size={usb_sector_size},physical_block_size={usb_sector_size}'])
        if firmware:
            import shutil
            code,template=firmware
            variables=OUT/(name+'-vars.fd')
            shutil.copyfile(template,variables)
            command.extend(['-drive',f'if=pflash,format=raw,readonly=on,file={code}',
                            '-drive',f'if=pflash,format=raw,file={variables}'])
        if cpu:command.extend(['-cpu',cpu])
        if rtc:command.extend(['-rtc',f'base={rtc}'])
        if nic:command.extend(['-nic',nic if isinstance(nic,str) else 'user,model=rtl8139'])
        if audio is not None:command.extend(['-audiodev',f'wav,id=myos-audio,path={audio}',
                                             '-device','sb16,audiodev=myos-audio'])
        if disk is not None:command.extend(['-drive',f'format=raw,file={disk},if=ide,index={disk_index}'])
        if asset_disk is not None:command.extend(['-drive',f'format=raw,file={asset_disk},if=ide,index=1'])
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
    def wait(self,text,timeout=None):
        if timeout is None:
            timeout=60 if text=='myos> ' else 15
        end=time.monotonic()+timeout
        while time.monotonic()<end:
            content=self.log.read_text()
            if text in content:return content
            if 'PANIC' in content and 'PANIC' not in text:raise AssertionError(content)
            time.sleep(.05)
        content=self.log.read_text()
        if text in content:return content
        raise AssertionError(f'{self.name}: missing {text!r}\n{content}')
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
        self.process.terminate()
        try:self.process.wait(timeout=5)
        except subprocess.TimeoutExpired:
            self.process.kill();self.process.wait(timeout=5)
        self.stream.close();self.connection.close();self.sock.unlink(missing_ok=True)

def parse_symbols(output):
    result={}
    aliases={}
    for line in output.splitlines():
        fields=line.split()
        if len(fields)!=3:continue
        name=fields[2]
        result[name]=int(fields[0],16)
        # GCC LTO promotes private objects to uniquely suffixed symbols.
        # An alias is safe only if exactly one actual symbol owns the name;
        # never guess between same-named statics from different source files.
        base,separator,suffix=name.rpartition('.lto_priv.')
        if separator and base and suffix.isdecimal():
            aliases.setdefault(base,[]).append(name)
    for base,names in aliases.items():
        if base not in result and len(names)==1:result[base]=result[names[0]]
    return result

def symbols():
    return parse_symbols(subprocess.check_output(['nm','build/kernel.elf'],text=True))

def boot_test(name,ram=64,keyboard=False,text=False):
    vm=VM(name,ram)
    try:
        content=vm.wait('myos> ')
        for marker in ('64-bit Long Mode','PASS: PMM, paging, heap, exhaustion/recovery',
                       'PASS: two CPU-bound tasks preempted; task exit',
                       'PASS: context registers and direction flag preserved',
                       'PASS: Ring 3 x87/XMM/MXCSR isolation, preemption and slot reset',
                       'PASS: Ring 3 processes, private CR3/data/stack, timer preemption',
                       'PASS: 13 user fault cases contained; kernel and peer survived',
                       'PASS: process kill/reap, capacity, PID and resource reuse',
                       'PASS: syscall ABI, 29 user checks, pointer validation, signed exit',
                       'PASS: user VM map/unmap/protect, W^X, faults and cleanup',
                       'PASS: Ring 3 threads, FS-base TLS, wait/wake and reap',
                       'PASS: user write/getpid/yield/exit; status=42, resources reclaimed',
                       'PASS: RAMFS paths, binary I/O, offsets, permissions and cleanup',
                       'PASS: graphics surface, clipping, font and redraw',
                       'PS/2 mouse initialized',
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
        # Fault builds run the same PROCESS_TESTS boot checks as the normal
        # image. Use its existing boot budget; error/rip/cr2 assertions stay.
        content=vm.wait('PANIC: '+marker,timeout=60)
        assert f'error=0x{error:016X}' in content,content
        if case in (2,3,4):assert 'cr2=0x0000000200000000' in content,content
        else:assert f"rip=0x{symbols()['fault_site']:016X}" in content,content
        print(f'PASS: {marker}, error={error:#x}',flush=True)
    finally:vm.close()

def main():
    parser=argparse.ArgumentParser();parser.add_argument('--suite',choices=['all','boot','memory','keyboard','exceptions','timer','processes','syscalls','filesystem','shell','storage','graphics','mouse','desktop','settings','calculator','snake','browser','audio','video','release','apps'],default='all');args=parser.parse_args()
    try:
        if args.suite=='all':
            from test_run_logged import main as logging_exercise
            logging_exercise()
        if args.suite in ('all','apps'):
            from test_tcp_state import main as tcp_state_exercise
            tcp_state_exercise()
        build()
        boot_test('framebuffer-64M',keyboard=args.suite in ('all','keyboard'))
        if args.suite in ('all','shell'):
            from test_shell import exercise
            exercise()
        if args.suite in ('all','graphics'):
            from test_graphics import exercise
            exercise()
        if args.suite in ('all','mouse'):
            from test_mouse import exercise
            exercise()
        if args.suite in ('all','desktop'):
            from test_desktop import exercise,low_memory
            exercise()
            low_memory()
        if args.suite in ('all','settings'):
            from test_settings import exercise,low_memory
            exercise()
            low_memory()
        if args.suite in ('all','calculator'):
            from test_calculator import exercise,low_memory
            exercise()
            low_memory()
        if args.suite in ('all','snake'):
            from test_snake import exercise,low_memory
            exercise()
            low_memory()
        if args.suite in ('all','audio'):
            from test_audio import main as audio_exercise
            audio_exercise()
        if args.suite in ('all','video'):
            from test_video import main as video_exercise
            video_exercise()
        if args.suite in ('all','browser'):
            from test_browser import main as browser_exercise,low_memory as browser_low_memory
            browser_exercise()
            browser_low_memory()
        if args.suite in ('all','release'):
            from test_desktop_release import exercise,low_memory
            exercise()
            low_memory()
        if args.suite in ('all','apps'):
            from test_app_runtime import exercise as app_exercise
            app_exercise()
            from test_elf_runtime import exercise as elf_exercise
            elf_exercise()
            from test_elf_dynamic import exercise as dynamic_exercise
            dynamic_exercise()
            from test_elf_tcp import main as tcp_exercise
            tcp_exercise()
            from test_tcp_link_loss import main as tcp_link_loss_exercise
            tcp_link_loss_exercise([])
            from test_socket_options import main as socket_options_exercise
            socket_options_exercise()
            from test_native_ui import main as native_ui_exercise
            native_ui_exercise()
            from test_platform_events import main as platform_events_exercise
            platform_events_exercise()
            from test_starboard_launcher import main as starboard_launcher_exercise
            starboard_launcher_exercise()
            for resolver_options in ([],['--offline'],['--no-entropy']):
                subprocess.run([sys.executable,'tests/test_netdb.py',*resolver_options],check=True)
            subprocess.run([sys.executable,'tests/test_cross_signal.py','user/elf-cross-signal.c'],check=True)
            subprocess.run([sys.executable,'tests/test_runtime_pack.py'],check=True)
            subprocess.run([sys.executable,'tests/test_asset_format.py'],check=True)
            for batch_test in ('test_asset_v2.py','test_runtime_asset.py','test_message.py','test_interface.py','test_pcm.py','test_audio_sink.py','test_media_policy.py','test_server.py'):
                subprocess.run([sys.executable,'tests/'+batch_test],check=True)
            for system_options in ([],['--network'],['--no-entropy'],['--launcher']):
                subprocess.run([sys.executable,'tests/test_starboard_system.py',*system_options],check=True)
            from test_native_tls import main as native_tls_exercise
            native_tls_exercise()
            from test_thread_services import main as thread_services_exercise
            thread_services_exercise()
            subprocess.run([sys.executable, "tests/test_thread_vm_ranges.py", "--boot-image", str(ROOT/"build/myos-current.img"), "--runtime", str(ROOT/"build/platform-runtime.mrt")],check=True)
        if args.suite in ('all','storage'):
            from test_storage import exercise
            exercise()
        if args.suite in ('all','memory'):
            # App-focused helpers may restore PROCESS_TESTS=0. Boot assertions
            # below require the instrumented image, regardless of helper order.
            build()
            for ram in (16,256):boot_test(f'ram-{ram}M',ram)
        if args.suite in ('all','exceptions'):
            fault_test(1,'Invalid Opcode',0)
            fault_test(6,'General Protection Fault',0x48)
            fault_test(2,'Page Fault',2)
            fault_test(3,'Page Fault',3)
            fault_test(4,'Page Fault',17)
        if args.suite in ('all','graphics','mouse','desktop','settings','calculator','snake','release'):
            build(text=True);boot_test('vga-fallback',keyboard=args.suite=='all',text=True)
            from test_shell import exercise
            exercise(text=True)
            from test_graphics import exercise as graphics_exercise
            graphics_exercise(text=True)
            if args.suite=='all':
                from test_native_ui import main as native_ui_exercise
                native_ui_exercise(text=True)
            if args.suite in ('all','mouse'):
                from test_mouse import exercise as mouse_exercise
                mouse_exercise(text=True)
            if args.suite in ('all','desktop'):
                from test_desktop import exercise as desktop_exercise
                desktop_exercise(text=True)
            if args.suite in ('all','settings'):
                from test_settings import exercise as settings_exercise
                settings_exercise(text=True)
            if args.suite in ('all','calculator'):
                from test_calculator import exercise as calculator_exercise
                calculator_exercise(text=True)
            if args.suite in ('all','snake'):
                from test_snake import exercise as snake_exercise
                snake_exercise(text=True)
            if args.suite in ('all','release'):
                from test_desktop_release import exercise as release_exercise
                release_exercise(text=True)
    finally:
        env=dict(os.environ,TEST_CASE="0",TEXT_ONLY="0",PROCESS_TESTS="0")
        subprocess.run(["bash","build.sh"],env=env,check=True)
    print('PASS: selected suite; normal framebuffer image restored',flush=True)

if __name__=='__main__':main()
