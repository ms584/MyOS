"""QEMU single-object dynamic ELF acceptance, including fail-closed startup."""
import argparse
import csv
import hashlib
import json
import re
from pathlib import Path
import struct
import subprocess
import sys
import time
from test_system import VM,OUT,ROOT
from test_shell import command,type_text
sys.path.insert(0,str(ROOT/'tools'))
from make_asset_disk import create
from create_data_disk import create_image

def implemented_exports():
    with (ROOT/'docs/PHASE331_IMPORTS.csv').open(newline='') as source:
        return {row['symbol'] for row in csv.DictReader(source)
                if row['status']=='implemented-tested'}

def digest(path):
    with path.open('rb') as f:return hashlib.file_digest(f,'sha256').hexdigest()

def long_command(vm,text,timeout=240):
    offset=len(vm.log.read_text());type_text(vm,text+'\n');end=time.monotonic()+timeout
    while time.monotonic()<end:
        result=vm.log.read_text()[offset:]
        assert 'PANIC' not in result,result
        if result.endswith('myos> '):return result
        time.sleep(.1)
    raise AssertionError((text,vm.log.read_text()[-4000:]))

def cancel_live(vm,name,marker,timeout=90,minimum_elapsed=0):
    offset=len(vm.log.read_text());begin=time.monotonic();type_text(vm,'elf live '+name+'\n')
    while time.monotonic()-begin<timeout:
        output=vm.log.read_text()[offset:]
        assert 'PANIC' not in output and not output.endswith('myos> '),output
        if marker in output:break
        time.sleep(.05)
    else:raise AssertionError((name,marker,vm.log.read_text()[-4000:]))
    assert time.monotonic()-begin>=minimum_elapsed
    vm.command('input-send-event',{'events':[
        {'type':'key','data':{'down':down,'key':{'type':'qcode','data':'esc'}}}
        for down in (True,False)]})
    end=time.monotonic()+5
    while time.monotonic()<end:
        output=vm.log.read_text()[offset:]
        assert 'PANIC' not in output,output
        if output.endswith('myos> '):
            assert 'ELF cancelled; all threads stopped' in output,output
            assert_live_resources(output)
            return output
        time.sleep(.05)
    raise AssertionError(('live cancellation unresponsive',vm.log.read_text()[-4000:]))

def assert_live_resources(output):
    match=re.search(r'ELF live: pages reclaimed; heap-before=(\d+) heap-after=(\d+)',output)
    assert match and match[1]==match[2],output

def fixtures():
    out=OUT/'elf-dynamic';out.mkdir(exist_ok=True)
    objects=[]
    for source in ('elf-dynamic','elf-dynamic-export'):
        obj=out/(source+'.o');objects.append(str(obj))
        subprocess.run(['nasm','-f','elf64',f'user/{source}.asm','-o',str(obj)],check=True)
    library=out/'fixture.so'
    subprocess.run(['ld','-shared','--hash-style=both','-z','now','-z','relro','-e','_start',
                    '-init','init_first','-o',str(library),*objects],check=True)
    original=library.read_bytes();phoff=struct.unpack_from('<Q',original,32)[0]
    phnum=struct.unpack_from('<H',original,56)[0]
    ph=[struct.unpack_from('<IIQQQQQQ',original,phoff+i*56) for i in range(phnum)]
    def fileoff(va):
        for p in ph:
            if p[0]==1 and p[3]<=va<p[3]+p[5]:return p[2]+va-p[3]
        raise ValueError(hex(va))
    dynamic=next(p for p in ph if p[0]==2);tags={};positions={}
    for off in range(dynamic[2],dynamic[2]+dynamic[5],16):
        tag,value=struct.unpack_from('<qQ',original,off)
        if not tag:break
        tags[tag]=value;positions[tag]=off
    rel=[]
    for start,size in ((tags[7],tags[8]),(tags[23],tags[2])):
        for off in range(fileoff(start),fileoff(start)+size,24):rel.append((off,*struct.unpack_from('<QQq',original,off)))
    assert {info&0xffffffff for _,_,info,_ in rel}=={1,6,7,8},rel
    variants={}
    def mutate(name,offset,fmt,value):
        data=bytearray(original);struct.pack_into(fmt,data,offset,value);variants[name]=data
    data=bytearray(original);struct.pack_into('<Q',data,40,0);struct.pack_into('<HHH',data,58,0,0,0);variants['sectionless']=data
    # Exercise both dynamic hash formats without a section table dependency.
    for name,tag in [('gnu-only',4),('sysv-only',0x6ffffef5)]:
        data=bytearray(original);struct.pack_into('<qQ',data,positions[tag],21,0);variants[name]=data
    symoff=fileoff(tags[6]);count=struct.unpack_from('<II',original,fileoff(tags[4]))[1]
    weak=next(symoff+i*24 for i in range(count) if original[symoff+i*24+4]>>4==2)
    mutate('missing',weak+4,'B',(original[weak+4]&15)|16)
    mutate('bad-rela-size',positions[9]+8,'Q',16)
    # A live opt-in must preserve failed-load page/heap rollback at low RAM.
    last_load=max(i for i,p in enumerate(ph) if p[0]==1 and p[6])
    mutate('live-oom',phoff+last_load*56+40,'Q',32*1024*1024)
    mutate('duplicate-tag',positions[4],'q',5)
    mutate('needed',positions[4],'q',1)
    mutate('unknown-reloc',rel[0][0]+8,'Q',99)
    mutate('bad-target',rel[0][0],'Q',2**64-1)
    text=next(p for p in ph if p[0]==1 and p[1]&1)
    mutate('text-target',rel[0][0],'Q',text[3])
    mutate('metadata-target',rel[0][0],'Q',tags[7])
    absolute=next(item for item in rel if item[2]&0xffffffff==1)
    mutate('bad-symbol',absolute[0]+8,'Q',((count+1)<<32)|1)
    data=bytearray(original)
    value=struct.unpack_from('<Q',original,symoff+(absolute[2]>>32)*24+8)[0]
    struct.pack_into('<Qq',data,absolute[0]+8,1,0x8000000000+value)
    variants['zero-symbol']=data
    data=bytearray(original);struct.pack_into('<qQ',data,positions[4],13,tags[12]);variants['finalizer']=data
    ctor=next(item for item in rel if item[1]==tags[25])
    data_segment=next(p for p in ph if p[0]==1 and p[1]&2)
    mutate('bad-constructor',ctor[0]+16,'q',data_segment[3])
    relro=next(p for p in ph if p[0]==0x6474e552)
    entry=struct.unpack_from('<Q',original,24)[0]
    data=bytearray(original);code=b'\x48\xb8'+struct.pack('<Q',0x8000000000+relro[3])+b'\xc6\x00\x00'
    data[fileoff(entry):fileoff(entry)+len(code)]=code;variants['relro-fault']=data
    for name,code in [('constructor-fault',b'\x0f\x0b'),('constructor-loop',b'\xeb\xfe')]:
        data=bytearray(original);off=fileoff(tags[12]);data[off:off+len(code)]=code;variants[name]=data
    platform_specs=[];fini_specs=[]
    # Test the native finalizer lifecycle, independently of Cobalt imports.
    for mode,name in ((1,'fini'),(2,'fini-only'),(3,'fini-array')):
        obj=out/(name+'.o');image=out/(name+'.so')
        subprocess.run(['gcc','-m64','-std=c11','-Os','-fPIC','-ffreestanding','-fno-builtin',
                        '-fno-stack-protector','-fno-asynchronous-unwind-tables','-fno-unwind-tables',
                        '-Wall','-Wextra','-Werror','-Wno-unused-function',f'-DFINI_MODE={mode}',
                        '-c','user/elf-finalizer.c','-o',str(obj)],check=True)
        fini_args=['-fini','fini_last'] if mode!=3 else []
        subprocess.run(['ld','-shared','--hash-style=both','-z','now','-z','relro',
                        '-e','_start','-init','init_first',*fini_args,'-o',str(image),str(obj)],check=True)
        fini_specs.append(f'{name}={image}')
        if mode!=1:continue
        fini_original=image.read_bytes()
        fini_phoff=struct.unpack_from('<Q',fini_original,32)[0]
        fini_phnum=struct.unpack_from('<H',fini_original,56)[0]
        fini_ph=[struct.unpack_from('<IIQQQQQQ',fini_original,fini_phoff+i*56) for i in range(fini_phnum)]
        def fini_fileoff(va):
            for segment in fini_ph:
                if segment[0]==1 and segment[3]<=va<segment[3]+segment[5]:return segment[2]+va-segment[3]
            raise ValueError(hex(va))
        fini_dyn=next(segment for segment in fini_ph if segment[0]==2)
        fini_tags={};fini_positions={}
        for offset in range(fini_dyn[2],fini_dyn[2]+fini_dyn[5],16):
            tag,val=struct.unpack_from('<qQ',fini_original,offset)
            if not tag:break
            fini_tags[tag]=val;fini_positions[tag]=offset
        assert fini_tags[28]==16 and 13 in fini_tags
        fini_bad_data=next(segment[3] for segment in fini_ph if segment[0]==1 and segment[1]&2)
        fini_variants={}
        for bad_name,tag,val in (('bad-fini',13,fini_bad_data),('bad-fini-array',26,2**64-1),
                                 ('wrapped-fini',13,2**64-0x8000000000+fini_tags[13]),
                                 ('wrapped-init',12,2**64-0x8000000000+fini_tags[12]),
                                 ('bad-fini-size',28,7),('huge-fini-size',28,129*8)):
            bad=bytearray(fini_original);struct.pack_into('<Q',bad,fini_positions[tag]+8,val)
            fini_variants[bad_name]=bad
        bad=bytearray(fini_original)
        for offset in range(fini_fileoff(fini_tags[7]),fini_fileoff(fini_tags[7])+fini_tags[8],24):
            target,info,_=struct.unpack_from('<QQq',bad,offset)
            if target==fini_tags[26]:
                assert info&0xffffffff==8
                struct.pack_into('<q',bad,offset+16,fini_bad_data);break
        else:raise AssertionError('missing fini array relocation')
        fini_variants['bad-fini-target']=bad
        for bad_name,va,code in (
                ('fini-fault',fini_tags[13],b'\x0f\x0b'),('fini-loop',fini_tags[13],b'\xeb\xfe'),
                ('fini-ctor-loop',fini_tags[12],b'\xeb\xfe'),
                ('fini-entry-loop',struct.unpack_from('<Q',fini_original,24)[0],b'\xeb\xfe'),
                ('fini-early-exit',struct.unpack_from('<Q',fini_original,24)[0],
                 b'\xb8\x03\x00\x00\x00\xbf\x2a\x00\x00\x00\xcd\x80\x0f\x0b')):
            bad=bytearray(fini_original);offset=fini_fileoff(va);bad[offset:offset+len(code)]=code
            fini_variants[bad_name]=bad
        for variant,bad in fini_variants.items():
            path=out/(variant+'.so');path.write_bytes(bad);fini_specs.append(f'{variant}={path}')
    for source,name in (('elf-platform-allocator','allocator'),('elf-platform-vm','vm')):
        obj=out/(source+'.o');image=out/(name+'.so')
        subprocess.run(['nasm','-f','elf64',f'user/{source}.asm','-o',str(obj)],check=True)
        subprocess.run(['ld','-shared','--hash-style=both','-z','now','-z','relro','-e','_start',
                        '-o',str(image),str(obj)],check=True)
        platform_specs.append(f'{name}={image}')
    pthread_obj=out/'elf-platform-pthread.o';pthread_image=out/'pthread.so'
    subprocess.run(['gcc','-m64','-std=c11','-Os','-fPIC','-ffreestanding','-fno-builtin',
                    '-fno-stack-protector','-fno-asynchronous-unwind-tables','-fno-unwind-tables',
                    '-Wall','-Wextra','-Werror','-c','user/elf-platform-pthread.c',
                    '-o',str(pthread_obj)],check=True)
    subprocess.run(['ld','-shared','--hash-style=both','-z','now','-z','relro','-e','_start',
                    '-o',str(pthread_image),str(pthread_obj)],check=True)
    platform_specs.append(f'pthread={pthread_image}')
    wait_obj=out/'elf-platform-wait.o';wait_image=out/'wait.so'
    subprocess.run(['gcc','-m64','-std=c11','-Os','-fPIC','-ffreestanding','-fno-builtin',
                    '-fno-stack-protector','-fno-asynchronous-unwind-tables','-fno-unwind-tables',
                    '-Wall','-Wextra','-Werror','-c','user/elf-platform-wait.c',
                    '-o',str(wait_obj)],check=True)
    subprocess.run(['ld','-shared','--hash-style=both','-z','now','-z','relro','-e','_start',
                    '-o',str(wait_image),str(wait_obj)],check=True)
    platform_specs.append(f'wait={wait_image}')
    system_obj=out/'elf-platform-system.o';system_image=out/'system.so'
    subprocess.run(['gcc','-m64','-std=c11','-Os','-fPIC','-ffreestanding','-fno-builtin',
                    '-fno-stack-protector','-fno-asynchronous-unwind-tables','-fno-unwind-tables',
                    '-Wall','-Wextra','-Werror','-c','user/elf-platform-system.c',
                    '-o',str(system_obj)],check=True)
    subprocess.run(['ld','-shared','--hash-style=both','-z','now','-z','relro','-e','_start',
                    '-o',str(system_image),str(system_obj)],check=True)
    platform_specs.append(f'system={system_image}')
    file_obj=out/'elf-platform-file.o';file_image=out/'file.so'
    subprocess.run(['gcc','-m64','-std=c11','-Os','-fPIC','-ffreestanding','-fno-builtin',
                    '-fno-stack-protector','-fno-asynchronous-unwind-tables','-fno-unwind-tables',
                    '-Wall','-Wextra','-Werror','-c','user/elf-platform-file.c',
                    '-o',str(file_obj)],check=True)
    subprocess.run(['ld','-shared','--hash-style=both','-z','now','-z','relro','-e','_start',
                    '-o',str(file_image),str(file_obj)],check=True)
    platform_specs.append(f'file={file_image}')
    socket_obj=out/'elf-platform-socketpair.o';socket_image=out/'socketpair.so'
    subprocess.run(['gcc','-m64','-std=c11','-Os','-fPIC','-ffreestanding','-fno-builtin',
                    '-fno-stack-protector','-fno-asynchronous-unwind-tables','-fno-unwind-tables',
                    '-Wall','-Wextra','-Werror','-c','user/elf-platform-socketpair.c',
                    '-o',str(socket_obj)],check=True)
    subprocess.run(['ld','-shared','--hash-style=both','-z','now','-z','relro','-e','_start',
                    '-o',str(socket_image),str(socket_obj)],check=True)
    platform_specs.append(f'socketpair={socket_image}')
    lifecycle_obj=out/'elf-platform-lifecycle.o';lifecycle_image=out/'lifecycle.so'
    subprocess.run(['gcc','-m64','-std=c11','-Os','-fPIC','-ffreestanding','-fno-builtin',
                    '-fno-stack-protector','-fno-asynchronous-unwind-tables','-fno-unwind-tables',
                    '-Wall','-Wextra','-Werror','-c','user/elf-platform-lifecycle.c',
                    '-o',str(lifecycle_obj)],check=True)
    subprocess.run(['ld','-shared','--hash-style=both','-z','now','-z','relro','-e','_start',
                    '-o',str(lifecycle_image),str(lifecycle_obj)],check=True)
    platform_specs.append(f'lifecycle={lifecycle_image}')
    sync_obj=out/'elf-platform-file-sync.o';sync_image=out/'file-sync.so'
    subprocess.run(['gcc','-m64','-std=c11','-Os','-fPIC','-ffreestanding','-fno-builtin',
                    '-fno-stack-protector','-fno-asynchronous-unwind-tables','-fno-unwind-tables',
                    '-Wall','-Wextra','-Werror','-c','user/elf-platform-file-sync.c',
                    '-o',str(sync_obj)],check=True)
    subprocess.run(['ld','-shared','--hash-style=both','-z','now','-z','relro','-e','_start',
                    '-o',str(sync_image),str(sync_obj)],check=True)
    platform_specs.append(f'file-sync={sync_image}')
    large=out/'large.bin';large.write_bytes(bytes(index%251 for index in range(131072)))
    specs=[f'dynamic={library}',*platform_specs,f'large={large}']
    for name,data in variants.items():
        path=out/(name+'.so');path.write_bytes(data);specs.append(f'{name}={path}')
    disk=out/'assets.img';disk.unlink(missing_ok=True);create(disk,specs)
    fini_disk=out/'finalizer-assets.img';fini_disk.unlink(missing_ok=True);create(fini_disk,fini_specs)
    return out,disk,fini_disk

def exercise_finalizers(disk):
    before=digest(disk);vm=VM('elf-finalizers',disk=disk,ram=16)
    try:
        vm.wait('myos> ')
        for name in ('fini','fini-only','fini-array'):
            for _ in range(2):
                output=command(vm,'elf live '+name,'ELF exited with status 41')
                assert output.count('ELF finalizer entry returned')==1,output
                assert output.count('ELF Ring 3 init/fini order and status PASS')==1,output
                assert_live_resources(output)
        for name in ('bad-fini','bad-fini-array','bad-fini-size','huge-fini-size','bad-fini-target','wrapped-fini','wrapped-init'):
            output=command(vm,'elf live '+name,'error: invalid executable')
            assert 'ELF live: started' not in output and 'finalizer entry returned' not in output,output
        output=command(vm,'elf live fini-fault','error: invalid executable')
        assert 'USER FAULT' in output and 'finalizer entry returned' in output,output
        assert_live_resources(output)
        for name,marker in (('fini-loop','ELF finalizer entry returned'),('fini-ctor-loop','ELF live: started'),
                            ('fini-entry-loop','ELF live: started')):
            output=cancel_live(vm,name,marker,timeout=15)
            assert 'ELF Ring 3 init/fini order and status PASS' not in output,output
        output=command(vm,'elf run fini-loop','error: app timed out')
        assert 'ELF finalizer entry returned' in output,output
        output=command(vm,'elf live fini-early-exit','ELF exited with status 42')
        assert 'ELF Ring 3 init/fini order and status PASS' not in output,output
        assert_live_resources(output)
        print('PASS: Ring 3 DT_FINI/reverse FINI_ARRAY, exactly-once order/status, validation, fault/kill/timeout/relaunch cleanup',flush=True)
    finally:vm.close()
    assert digest(disk)==before

def exercise(cobalt=None):
    out,disk,fini_disk=fixtures();before=digest(disk);vm=VM('elf-dynamic',disk=disk,ram=16)
    try:
        vm.wait('myos> ')
        output=command(vm,'elf link dynamic','staged image released; not executed')
        assert 'relative=2 absolute=1 global=2 jump=1 skipped=0' in output,output
        assert 'constructor executed' not in output,output
        for name in ('dynamic','sectionless','gnu-only','sysv-only','zero-symbol','finalizer'):
            output=command(vm,'elf run '+name,'ELF exited with status 32')
            assert 'constructor executed in Ring 3' in output,output
            assert 'ELF dynamic: four relocations' in output,output
        for name in ('bad-rela-size','duplicate-tag','bad-target','text-target','metadata-target','bad-symbol','bad-constructor'):
            output=command(vm,'elf run '+name,'error: invalid executable')
            assert 'constructor executed' not in output,output
        for name in ('missing','needed','unknown-reloc'):
            output=command(vm,'elf run '+name,'unsupported runtime feature; not executed')
            assert 'constructor executed' not in output,output
        output=command(vm,'elf run allocator','ELF exited with status 33')
        assert 'ELF platform allocator semantics PASS' in output,output
        assert all(f'ELF platform: {name}' in output for name in
                   ('aligned_alloc','calloc','free','malloc','malloc_usable_size','posix_memalign','realloc')),output
        output=command(vm,'elf run vm','ELF exited with status 34')
        assert 'ELF POSIX VM and per-thread errno PASS' in output,output
        assert all(f'ELF platform: {name}' in output for name in
                   ('__errno_location','madvise','mincore','mmap','mprotect','msync','munmap')),output
        output=command(vm,'elf run pthread','ELF exited with status 35')
        assert 'ELF pthread ABI, clocks, masks, lifecycle and TLS PASS' in output,output
        output=command(vm,'elf run wait','ELF exited with status 36')
        assert 'ELF descriptor lifetime, eventfd, pipe, poll, epoll and FIONREAD PASS' in output,output
        output=command(vm,'elf run system','ELF exited with status 37')
        assert 'ELF system, constants and honest optional-feature contracts PASS' in output,output
        output=command(vm,'elf run file','ELF exited with status 38')
        assert 'ELF regular-file descriptor ABI and shared lifetime PASS' in output,output
        output=command(vm,'elf run socketpair','ELF exited with status 40')
        assert 'ELF socketpair duplex, shutdown, readiness and cancellation PASS' in output,output
        command(vm,'touch /home/live-open') # Include persistent RAMFS allocation in baseline.
        cancel_live(vm,'lifecycle','ELF live runtime beyond 60 seconds PASS',minimum_elapsed=60)
        command(vm,'rm /home/live-open') # Kernel handle references must be gone.
        for _ in range(2):
            command(vm,'touch /home/live-open')
            cancel_live(vm,'lifecycle','ELF live workers blocked PASS',timeout=15)
            command(vm,'rm /home/live-open')
        cancel_live(vm,'constructor-loop','ELF live: started',timeout=15)
        output=command(vm,'elf live dynamic','ELF exited with status 32')
        assert_live_resources(output)
        output=command(vm,'elf live relro-fault','error: invalid executable')
        assert 'USER FAULT' in output,output
        assert_live_resources(output)
        command(vm,'elf live live-oom','error: out of memory')
        print('PASS: >60s native lifecycle, Escape cancel with blocked workers, repeated launch, page/heap/file reclamation',flush=True)
        output=command(vm,'elf run relro-fault','error: invalid executable')
        assert 'vector=14 error=0x0000000000000007' in output and 'ELF RELRO: protected' in output,output
        output=command(vm,'elf run constructor-fault','error: invalid executable')
        assert 'vector=6 error=0x0000000000000000' in output and 'ELF dynamic: four relocations' not in output,output
        command(vm,'elf run constructor-loop','error: app timed out')
        for _ in range(3):command(vm,'elf run dynamic','ELF exited with status 32')
        command(vm,'run /bin/hello','App exited with status 42')
        print('PASS: four dynamic relocation types; SysV/GNU hash; sectionless startup; Ring 3 constructors; RELRO fault; invalid/missing-symbol rollback',flush=True)
    finally:vm.close()
    assert digest(disk)==before
    exercise_finalizers(fini_disk)
    data=out/'file-sync-data.img';data.unlink(missing_ok=True);create_image(data,large=True)
    for name,marker in (('file-sync-commit','ELF file fsync committed binary data PASS'),
                        ('file-sync-reboot','ELF file fsync reboot persistence PASS')):
        vm=VM(name,disk=data,asset_disk=disk)
        try:
            vm.wait('myos> ')
            output=command(vm,'elf run file-sync','ELF exited with status 39')
            assert marker in output,output
        finally:vm.close()
    assert digest(disk)==before
    print('PASS: native file fsync/fdatasync binary data survives reboot on disposable v2 disk',flush=True)
    if cobalt:
        audit=json.loads((ROOT/'docs/PHASE33_COBALT_AUDIT.json').read_text())
        assert digest(cobalt)==audit['library_sha256'],'wrong Cobalt build'
        disk=out/'cobalt.img';disk.unlink(missing_ok=True);create(disk,[f'cobalt={cobalt}']);before=digest(disk)
        vm=VM('elf-dynamic-cobalt',disk=disk,ram=256)
        try:
            vm.wait('myos> ')
            output=long_command(vm,'elf link cobalt')
            assert 'relative=117653 absolute=20 global=17 jump=626' in output,output[-4000:]
            exports=implemented_exports()
            expected=[name for name in audit['undefined_symbols']
                      if not name.startswith('_ZTH') and name not in exports]
            assert f'missing={len(expected)} weak-zero=1' in output,output[-4000:]
            assert 'ELF RELRO: protected' in output,output[-4000:]
            assert 'staged image released; not executed' in output,output[-4000:]
            imports=[line.split(': ',1)[1] for line in output.splitlines() if line.startswith('ELF missing: ')]
            assert sorted(imports)==sorted(expected)
            command(vm,'run /bin/hello','App exited with status 42')
            print(f'PASS: actual Cobalt mapped and relocations processed in MyOS; {len(exports)} tested runtime imports resolved, {len(expected)} missing imports block execution; all staged pages reclaimed',flush=True)
        finally:vm.close()
        assert digest(disk)==before
        vm=VM('elf-dynamic-cobalt-oom',disk=disk,ram=64)
        try:
            vm.wait('myos> ')
            output=long_command(vm,'elf link cobalt')
            assert 'error: out of memory' in output,output
            command(vm,'run /bin/hello','App exited with status 42')
            print('PASS: actual large image OOM at 64 MiB rolls back and leaves shell/apps usable',flush=True)
        finally:vm.close()

if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('--cobalt',type=Path);exercise(p.parse_args().cobalt)
