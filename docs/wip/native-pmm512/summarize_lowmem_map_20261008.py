from pathlib import Path
import re
path=Path('/home/xytron404/pmm-syscall-R4sQZZ/fault3-inspect-kernel.map')
items=[]
for match in re.finditer(r' \.bss\.(\S+)\s+(0x[0-9a-fA-F]+)\s+(0x[0-9a-fA-F]+)',path.read_text()):
 name,address,size=match.groups();items.append((int(size,16),address,name))
for size,address,name in sorted(items,reverse=True)[:18]:print(size,address,name)
