import sys,time
from pathlib import Path
root=Path('/home/xytron404/fs-native-VzEALo')
sys.path.insert(0,str(root/'tests'))
import test_system as test
test.build(3)
vm=test.VM('fault-3-timeout-audit')
start=time.monotonic()
try:
 content=vm.wait('PANIC: Page Fault',timeout=60)
 assert 'error=0x0000000000000003' in content,content
 assert 'cr2=0x0000000200000000' in content,content
 print('PASS: exact original fault-3 error/address; elapsed',time.monotonic()-start,flush=True)
finally:
 vm.close()
