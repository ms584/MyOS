"""Live MyOS HTTPS search and result-navigation acceptance test."""
import sys
sys.path.insert(0,'tests')
from test_system import VM,symbols
from test_shell import type_text
from test_desktop import move_to,button,wait_count
vm=VM('phase29-gui-live',nic=True,cpu='max')
try:
    vm.wait('myos> ',30)
    type_text(vm,'desktop\n');vm.wait('DESKTOP READY')
    vm.key('7');vm.wait('WINDOW OPEN 7')
    type_text(vm,'valid-isrgrootx1 letsencrypt\n')
    log=vm.wait('BROWSER: Search results (verified HTTPS)',80)
    vm.wait('BROWSER: valid-isrgrootx1 letsencrypt at DuckDuckGo',10)
    addr=symbols()
    raw=vm.memory(addr['lines'],160*(68+192+1))
    target=b'https://valid-isrgrootx1.letsencrypt.org/'
    assert target in raw,raw[:3000]
    width=68+192+1
    result=None
    for index in range(160):
        item=raw[index*width:(index+1)*width]
        href=item[68:68+192].split(b'\0',1)[0]
        if href==target and b'valid-isrgrootx1' in item[:68]:result=index;break
    assert result is not None,'search result link not present in rendered lines'
    while True:
        scroll=int.from_bytes(vm.memory(addr['scroll'],4),'little')
        if result<scroll+14:break
        vm.key('down')
    row=result-scroll
    move_to(vm,addr,245,287+18*row);button(vm,True)
    vm.wait('BROWSER: Page loaded (verified HTTPS)',80)
    vm.wait('BROWSER: valid-isrgrootx1.letsencrypt.org',10)
    button(vm,False)
    move_to(vm,addr,225,200);button(vm,True)
    wait_count(vm,'BROWSER: valid-isrgrootx1 letsencrypt at DuckDuckGo',2)
    wait_count(vm,'BROWSER: Search results (verified HTTPS)',2)
    button(vm,False)
    print('PASS: verified live GUI search, result click, HTTPS page and Back')
finally:vm.close()
