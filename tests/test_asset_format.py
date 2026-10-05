"""Private v1/v2 package and real kernel parser ASan/UBSan tests."""
from pathlib import Path
import subprocess
import tempfile
import struct
import sys
work=Path(__file__).resolve().parent
root=work.parent
sys.path.insert(0,str(root/'tools'))
from make_asset_disk import create
out=Path(tempfile.mkdtemp(prefix='asset-format-',dir=root/'build'))
data=out/'data.bin';data.write_bytes(b'payload')
runtime=out/'runtime.mrt';runtime.write_bytes(bytes(16))
specs=[name+'='+str(data) for name in ('root.txt','content/fonts/font.bin','content/icu/data.bin','content/cert.pem')]
good=out/'good.img';create(good,specs,runtime=runtime)
v1=out/'v1.img';create(v1,['root.txt='+str(data)],runtime=None)
check=out/'check'
subprocess.run(['gcc','-std=c11','-O1','-g','-fsanitize=address,undefined',
 '-fno-omit-frame-pointer','-Wall','-Wextra','-Werror','-Wno-misleading-indentation',
 '-I'+str(root/'include'),str(work/'asset-format-check.c'),'-o',str(check)],check=True)
for mode,path in [('good',good),('oom',good),('v1',v1)]:
 subprocess.run([str(check),str(path),mode],check=True)
# Independent frozen v1 wire representation, not the new writer called twice.
legacy=bytearray(1024*1024)
struct.pack_into('<16sIIQ',legacy,0,b'MYOS-ASSET-v1',1,1,len(legacy))
struct.pack_into('<48sQQ',legacy,512,b'root.txt',4096,7)
legacy[4096:4103]=b'payload'
assert bytes(legacy)==v1.read_bytes(),'v1 encoding changed'
many=out/'many.img';create(many,['root.txt='+str(data)]+
 [f'file{i:04d}.bin='+str(data) for i in range(1023)],runtime=runtime)
subprocess.run([str(check),str(many),'many'],check=True)
original=good.read_bytes()
def mutation(name,modify):
 data=bytearray(original);modify(data);path=out/(name+'.img');path.write_bytes(data)
 subprocess.run([str(check),str(path),'bad'],check=True)
mutation('magic',lambda b:b.__setitem__(0,0))
mutation('count-zero',lambda b:struct.pack_into('<I',b,20,0))
mutation('count-overflow',lambda b:struct.pack_into('<I',b,20,1025))
mutation('volume-overflow',lambda b:struct.pack_into('<Q',b,24,(1<<64)-1))
mutation('offset-overflow',lambda b:struct.pack_into('<Q',b,512+240,(1<<64)-1))
mutation('size-overflow',lambda b:struct.pack_into('<Q',b,512+248,(1<<64)-1))
mutation('runtime-overlap',lambda b:struct.pack_into('<Q',b,32,struct.unpack_from('<Q',b,512+240)[0]))
mutation('runtime-half-record',lambda b:struct.pack_into('<Q',b,40,0))
mutation('duplicate',lambda b:b.__setitem__(slice(768,768+240),b[512:512+240]))
mutation('unterminated',lambda b:b.__setitem__(slice(512,512+240),b'a'*240))
mutation('dot-component',lambda b:b.__setitem__(slice(512,512+240),b'content/../bad\0'.ljust(240,b'\0')))
def collision(b):
 for index,name in enumerate((b'a',b'a-',b'a/x')):
  b[512+index*256:512+index*256+240]=(name+b'\0').ljust(240,b'\0')
mutation('nonadjacent-file-directory',collision)
for names in (['../bad'],['content//x'],['a','a/x'],['a'*240],['/root']):
 try:create(out/('rejected-'+str(len(list(out.iterdir())))+'.img'),[name+'='+str(data) for name in names],runtime=runtime)
 except ValueError:pass
 else:raise AssertionError(('invalid package names accepted',names))
try:create(good,specs,runtime=runtime)
except FileExistsError:pass
else:raise AssertionError('existing disk overwritten')
print('PASS: PRIVATE asset v2 kernel parser/package ASan+UBSan; v1 retained, malformed bounds/overlap/names/OOM; exclusive writer.',out,flush=True)
