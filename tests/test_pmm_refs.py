"""Actual native PMM bootstrap/retain/exhaustion scope, not web acceptance."""
from test_system import VM
from pathlib import Path
for ram in (16,64,256):
 vm=VM('pmm-refs-'+str(ram),ram=ram)
 try:
  vm.wait('PASS: PMM, paging, heap, exhaustion/recovery',timeout=90)
  vm.wait('myos> ',timeout=90)
 finally:vm.close()
 print('PASS native retained-frame lifetime, invalid retain, paging and full exhaustion/recovery RAM',ram,flush=True)
