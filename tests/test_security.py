"""Phase 28 native time and entropy preconditions in QEMU."""
import datetime,re,sys
sys.path.insert(0,'tests')
from test_system import VM
from test_network import send
def check(name,cpu=None,rtc=None,valid=True):
    vm=VM(name,cpu=cpu,rtc=rtc)
    try:
        log=vm.wait('myos> ',30)
        send(vm,'security')
        log=vm.wait('Security: UTC ',8)
        status=log.split('Security: UTC ')[-1].split('\n')[0]
        if valid:
            value=re.match(r'(\d+), RDRAND ready, HTTPS prerequisites ready',status)
            assert value,status
            now=int(datetime.datetime.now(datetime.timezone.utc).timestamp())
            assert abs(int(value.group(1))-now)<180,(status,now)
        else:
            assert 'HTTPS blocked' in status,status
        return status
    finally:vm.close()
if __name__=='__main__':
    print('normal:',check('phase28-cpu-max',cpu='max'))
    print('old clock:',check('phase28-old-clock',cpu='max',rtc='2010-01-01',valid=False))
