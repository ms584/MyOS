"""Optional live QEMU probe for the current YouTube HTTPS page.

This proves verified HTTPS reaches a response. It is deliberately not a
YouTube playback acceptance test. External site content may change.
"""
import sys,time
sys.path.insert(0,'tests')
from test_system import VM
from test_shell import type_text
from test_browser import type_url

def main():
    vm=VM('phase32-youtube-live',ram=128,nic=True,cpu='max')
    try:
        vm.wait('myos> ',30)
        type_text(vm,'desktop\n')
        vm.wait('DESKTOP READY',15)
        vm.key('7')
        vm.wait('WINDOW OPEN 7',10)
        type_url(vm,'https://www.youtube.com/')
        vm.wait('BROWSER: HTTPS page exceeds 64 KiB',90)
        assert 'PANIC' not in vm.log.read_text()
        print('PASS: verified YouTube HTTPS reached response-size gate; playback still unsupported',flush=True)
    finally:
        vm.close()

if __name__=='__main__':main()
