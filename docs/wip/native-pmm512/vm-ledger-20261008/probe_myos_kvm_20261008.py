"""Boot the unchanged normal image using non-root KVM, then shut down QEMU."""
from pathlib import Path
import sys
sys.path.insert(0, str(Path.cwd() / 'tests'))
from qemu_kvm_optin_20261008 import install
install()
from test_system import VM
vm = VM('kvm-normal-smoke', ram=512, cpu='max')
try:
    vm.wait('myos> ', timeout=60)
    status = vm.command('query-kvm')
    assert status == {'enabled': True, 'present': True}, status
    print('KVM_NORMAL_BOOT_PASS', status, flush=True)
finally:
    vm.close()
