"""Run an unchanged test script under explicit non-root KVM."""
from pathlib import Path
import runpy
import sys
sys.path.insert(0, str(Path.cwd() / 'tests'))
from qemu_kvm_optin_20261008 import install
install()
script = Path(sys.argv[1]).resolve()
assert script.is_relative_to((Path.cwd() / 'tests').resolve())
sys.argv = [str(script), *sys.argv[2:]]
runpy.run_path(str(script), run_name='__main__')
