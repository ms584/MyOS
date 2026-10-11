"""Real virtual-device absolute pointer, buttons and PS/2 fallback; no web acceptance."""
import argparse,time,json,hashlib
from pathlib import Path
from unittest.mock import patch
import test_system
from test_system import VM,symbols
from test_shell import command,type_text
from test_graphics import pixels
p=argparse.ArgumentParser();p.add_argument('--without-vmport',action='store_true');p.add_argument('--uefi',action='store_true');a=p.parse_args()
original=test_system.subprocess.Popen
def launch(cmd,*args,**kwargs):
    if a.without_vmport:cmd=list(cmd)+['-machine','pc,vmport=off']
    return original(cmd,*args,**kwargs)
options={}
if a.uefi:options=dict(ram=128,boot_image=Path('build/myos-uefi.img').resolve(),boot_bus='ide',firmware=(Path('/usr/share/OVMF/OVMF_CODE_4M.fd'),Path('/usr/share/OVMF/OVMF_VARS_4M.fd')))
with patch.object(test_system.subprocess,'Popen',launch):vm=VM(('mouse-absolute-unavailable' if a.without_vmport else 'mouse-absolute')+('-uefi' if a.uefi else ''),**options)
try:
    vm.wait('myos> ')
    # Existing default relative input still works before opting in.
    vm.command('input-send-event',{'events':[{'type':'rel','data':{'axis':'x','value':40}}]});time.sleep(.05)
    command(vm,'mouse','Mouse: x=440 y=300 buttons=0')
    result=command(vm,'mouse absolute','Mouse: ')
    if a.without_vmport:
        assert 'absolute virtual device unavailable; PS/2 retained' in result,result
        vm.command('input-send-event',{'events':[{'type':'rel','data':{'axis':'x','value':-40}}]});time.sleep(.05)
        command(vm,'mouse','Mouse: x=400 y=300 buttons=0');command(vm,'pwd','/\n')
        print('PASS: absent virtual port retains default PS/2 and responsive shell',flush=True)
    else:
        assert 'absolute virtual coordinates enabled' in result,result
        devices=vm.command('query-mice');assert any(d['current'] and d['absolute'] for d in devices),devices
        type_text(vm,'gfx\n');vm.wait('GRAPHICS DEMO READY')
        def pos(x,y):
            vm.command('input-send-event',{'events':[{'type':'abs','data':{'axis':'x','value':x}},{'type':'abs','data':{'axis':'y','value':y}}]});time.sleep(.04)
        def button(down):vm.command('input-send-event',{'events':[{'type':'btn','data':{'button':'left','down':down}}]});time.sleep(.04)
        pos(16384,16384)
        image=Path('build/system-tests/mouse-absolute.ppm');vm.command('screendump',{'filename':str(image)})
        w,h,sample=pixels(image);assert (w,h)==(800,600);assert sample(400,300)==(16,16,16),sample(400,300)
        button(True);vm.wait('MOUSE LEFT DOWN');vm.command('screendump',{'filename':str(image)})
        _,_,sample=pixels(image);assert sample(401,302)==(245,182,66),sample(401,302);button(False)
        # Small movements near centre and corners cannot accumulate drift or acceleration.
        for x,y in [(16424,16424),(16384,16384),(0,0),(32767,32767),(0,32767),(32767,0),(16384,16384)]:
            pos(x,y)
            values=symbols()
            actual=tuple(int.from_bytes(vm.memory(values[n],4),'little',signed=True) for n in ['x_position.lto_priv.0','y_position.lto_priv.0'])
            expected=tuple(((v*65535//32767)*(limit-1)+32767)//65535 for v,limit in [(x,800),(y,600)])
            assert actual==expected,(x,y,actual,expected)
        vm.key('ret');vm.wait('Graphics demo closed\nmyos> ')
        command(vm,'mouse relative','Mouse: relative PS/2 enabled')
        assert not any(d['current'] and d['absolute'] for d in vm.command('query-mice'))
        vm.command('input-send-event',{'events':[{'type':'rel','data':{'axis':'x','value':10}},{'type':'rel','data':{'axis':'y','value':-10}}]});time.sleep(.05)
        command(vm,'mouse','Mouse: x=410 y=290 buttons=0');command(vm,'pwd','/\n')
        print('PASS: real absolute virtual pointer centre/corners/small moves/buttons, keyboard exit and return to PS/2',flush=True)
finally:vm.close()
