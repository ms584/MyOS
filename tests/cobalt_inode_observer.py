"""Bounded live/post-close inode metadata observer. No file bodies/heap contents/paths exported.
Requires matching normal kernel, reads only the128x80-byte low-memory inode table.
"""
import hashlib,struct,subprocess
from pathlib import Path
class InodeObserver:
    def __init__(self,vm):
        self.vm=vm;root=Path.cwd();kernel=(root/'build/kernel.bin').read_bytes()
        assert (root/'build/myos-current.img').read_bytes()[17*512:17*512+len(kernel)]==kernel
        matches=[]
        for line in subprocess.check_output(['nm','-S','build/kernel.elf'],text=True).splitlines():
            fields=line.split()
            if len(fields)==4 and fields[3].split('.lto_priv.')[0]=='nodes':matches.append(fields)
        assert len(matches)==1
        self.address=int(matches[0][0],16);self.size=int(matches[0][1],16)
        assert self.size==128*80 and self.address+self.size<0x80000
    def capture(self):
        self.vm.command('stop')
        try:
            assert self.vm.command('query-status')['status']=='paused'
            data=self.vm.memory(self.address,self.size)
        finally:self.vm.command('cont')
        rows=[]
        for i in range(128):
            used,unlinked,parent,kind,refs,size,capacity,name,body,shared=struct.unpack_from('<BBxxIIIQQ32sQQ',data,i*80)
            assert used in (0,1)
            if not used:continue
            assert unlinked in (0,1) and parent<128 and kind in (1,2)
            if shared:
                # Native shm_resize updates size; node.data/capacity stay zero.
                # Backing pages belong to PMM, not RAMFS file-buffer capacity.
                assert i>=64 and kind==1 and not body and capacity==0 and size<=8*1024*1024
            elif kind==1:
                assert size<=capacity<=65536
            rows.append(dict(index=i,unlinked=bool(unlinked),parent=parent,type=kind,references=refs,file_bytes=size,capacity_bytes=capacity,allocated_file_data=bool(body),shared=bool(shared),name_sha256=hashlib.sha256(name.split(b'\0')[0]).hexdigest()))
        return dict(nodes=rows,linked_file_buffer_capacity=sum(r['capacity_bytes'] for r in rows if not r['unlinked'] and r['allocated_file_data'] and not r['shared']),unlinked_file_buffer_capacity=sum(r['capacity_bytes'] for r in rows if r['unlinked'] and r['allocated_file_data'] and not r['shared']),scope='Bounded inode metadata/capacity only, names hashed; no file contents, raw heap, cookies or keys; does not assign non-file heap allocations.')
