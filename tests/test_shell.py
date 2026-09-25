"""Exercise the real shell using QEMU PS/2 events, not direct function calls."""
import time
from test_system import VM,symbols

def type_text(vm,text):
    plain={' ':'spc','/':'slash','.':'dot','-':'minus','\n':'ret','\b':'backspace'}
    for ch in text:
        code=plain.get(ch,ch.lower());codes=['shift',code] if ch.isupper() else [code]
        def event(c,down):return {'type':'key','data':{'down':down,'key':{'type':'qcode','data':c}}}
        vm.command('input-send-event',{'events':[event(c,True) for c in codes]})
        time.sleep(.012)
        vm.command('input-send-event',{'events':[event(c,False) for c in reversed(codes)]})
        time.sleep(.012)

def command(vm,text,expected=''):
    offset=len(vm.log.read_text());type_text(vm,text+'\n');end=time.monotonic()+30
    while time.monotonic()<end:
        output=vm.log.read_text()[offset:]
        if output.endswith('myos> '):
            result=output.split('\n',1)[1]
            assert expected in result,(text,expected,output)
            assert 'PANIC' not in output,output
            return result
        time.sleep(.03)
    raise AssertionError((text,vm.log.read_text()[offset:]))

def exercise(text=False):
    vm=VM('shell-vga' if text else 'shell-framebuffer')
    try:
        vm.wait('myos> ')
        command(vm,'help','write PATH TEXT')
        command(vm,'pwd','/\n')
        command(vm,'ls','etc/\n')
        command(vm,'cat /etc/motd','Welcome to MyOS RAMFS.')
        command(vm,'mkdir /home/demo')
        command(vm,'cd /home/demo')
        command(vm,'pwd','/home/demo\n')
        command(vm,'write note Hello MyOS')
        command(vm,'touch note')
        command(vm,'cat note','Hello MyOS\n')
        command(vm,'stat note','file 10 bytes')
        command(vm,'ls .','note\n')
        command(vm,'cd note','error: not a directory')
        command(vm,'rm .','error: busy')
        command(vm,'rm /home','error: busy')
        command(vm,'write note short')
        command(vm,'cat note','short\n')
        command(vm,'cat missing','error: not found')
        command(vm,'mkdir note','error: already exists')
        command(vm,'touch a extra','error: too many arguments')
        command(vm,'stat a','error: not found')
        command(vm,'echo hellx\bo','hello\n')
        command(vm,'\b\bpwd','/home/demo\n')
        command(vm,'echo '+('x'*110)+'\bY',('x'*109)+'Y\n')
        command(vm,'write overflow '+('a'*256),'error: line too long; command discarded')
        command(vm,'stat overflow','error: not found')
        command(vm,'unknown','error: unknown command')
        command(vm,'mem','Free physical pages: ')
        command(vm,'uptime','Uptime (approx seconds): ')
        command(vm,'rm note')
        command(vm,'cd ..')
        command(vm,'pwd','/home\n')
        command(vm,'rm demo')
        command(vm,'cd')
        command(vm,'pwd','/\n')
        command(vm,'clear')
        raw=vm.memory(symbols()['cells'],4000);size=(80*25 if text else 100*37)
        assert raw[:6]==b'myos> ' and raw[6:size]==b' '*(size-6),raw[:200]
        print('PASS: shell PS/2 commands, paths, file lifecycle, editing, limits and clear'+(' (VGA)' if text else ''),flush=True)
    finally:vm.close()
