"""Native single-module x64 ELF TLS; not renderer/Cobalt acceptance."""
import argparse
from pathlib import Path
import struct
import subprocess
from test_system import VM,OUT
from test_elf_dynamic import long_command,cancel_live,assert_live_resources
from make_asset_disk import create


def fixtures():
    out=OUT/'native-tls';out.mkdir(exist_ok=True);specs=[]
    for model,dialect in (('gd','gnu'),('desc','gnu2'),('ie','gnu'),('ld','gnu')):
        for mode in (range(5) if model=='gd' else (0,)):
            name=f'{model}{mode}';obj=out/(name+'.o');image=out/(name+'.so')
            subprocess.run(['gcc','-m64','-std=c11','-Os','-fPIC','-ffreestanding','-fno-builtin',
                            '-mno-red-zone','-fno-stack-protector','-fno-asynchronous-unwind-tables',
                            '-fno-unwind-tables','-Wall','-Wextra','-Werror','-Wno-misleading-indentation',
                            '-Itests/emutls-abi',f'-DMODE={mode}',f'-mtls-dialect={dialect}',
                            '-ftls-model='+{'ie':'initial-exec','ld':'local-dynamic'}.get(model,'global-dynamic'),
                            '-c','user/elf-native-tls.c','-o',str(obj)],check=True)
            subprocess.run(['ld','-shared','--hash-style=both','-z','now','-z','relro','-e','_start',
                            '-init','tls_constructor','-fini','tls_finalizer','-o',str(image),str(obj)],check=True)
            specs.append(f'{name}={image}')
            rel=subprocess.check_output(['readelf','-rW',str(image)],text=True)
            assert 'R_X86_64_TPOFF64' in rel
            if model=='gd':assert 'R_X86_64_DTPMOD64' in rel and 'R_X86_64_DTPOFF64' in rel
            if model=='desc':assert 'R_X86_64_TLSDESC' in rel
            if model=='ld':assert 'R_X86_64_DTPMOD64' in rel
    obj=out/'zero.o';image=out/'zero.so'
    subprocess.run(['gcc','-m64','-std=c11','-Os','-fPIC','-ffreestanding','-fno-builtin',
                    '-mno-red-zone','-fno-stack-protector','-fno-asynchronous-unwind-tables',
                    '-fno-unwind-tables','-ftls-model=initial-exec','-Wall','-Wextra','-Werror',
                    '-c','user/elf-native-tls-zero.c','-o',str(obj)],check=True)
    subprocess.run(['ld','-shared','--hash-style=both','-z','now','-z','relro','-e','_start',
                    '-o',str(image),str(obj)],check=True);specs.append(f'zero={image}')
    original=(out/'gd0.so').read_bytes()
    phoff=struct.unpack_from('<Q',original,32)[0];phnum=struct.unpack_from('<H',original,56)[0]
    ph=[struct.unpack_from('<IIQQQQQQ',original,phoff+i*56) for i in range(phnum)]
    tls_index=next(i for i,p in enumerate(ph) if p[0]==7);tls=ph[tls_index];pos=phoff+tls_index*56
    variants={};invalid=[];unsupported=[]
    def mutate(name,offset,fmt,value,support=False):
        data=bytearray(original);struct.pack_into('<'+fmt,data,offset,value);variants[name]=data
        (unsupported if support else invalid).append(name)
    mutate('large-tls',pos+40,'Q',65536,True)
    mutate('huge-align',pos+48,'Q',8192,True)
    mutate('bad-align',pos+48,'Q',3)
    mutate('bad-flags',pos+4,'I',6)
    mutate('bad-file-size',pos+32,'Q',tls[6]+1)
    mutate('bad-template',pos+8,'Q',len(original)-1)
    mutate('bad-template-va',pos+16,'Q',128*1024*1024-1)
    spare=next(i for i,p in enumerate(ph) if p[0]==0x6474e551)
    data=bytearray(original);data[phoff+spare*56:phoff+(spare+1)*56]=original[pos:pos+56]
    variants['duplicate-tls']=data;invalid.append('duplicate-tls')
    def fileoff(va):
        for p in ph:
            if p[0]==1 and p[3]<=va<p[3]+p[5]:return p[2]+va-p[3]
        raise AssertionError(hex(va))
    dynamic=next(p for p in ph if p[0]==2);tags={}
    for offset in range(dynamic[2],dynamic[2]+dynamic[5],16):
        tag,value=struct.unpack_from('<qQ',original,offset)
        if not tag:break
        tags[tag]=value
    count=struct.unpack_from('<II',original,fileoff(tags[4]))[1];symoff=fileoff(tags[6])
    tls_symbol=next(symoff+i*24 for i in range(count) if original[symoff+i*24+4]&15==6)
    mutate('outside-symbol',tls_symbol+8,'Q',tls[6])
    mutate('external-module',tls_symbol+6,'H',0,True)
    reloc=[]
    for start,size in ((tags[7],tags[8]),(tags[23],tags[2])):
        for offset in range(fileoff(start),fileoff(start)+size,24):
            reloc.append((offset,*struct.unpack_from('<QQq',original,offset)))
    dtp=next(r for r in reloc if r[2]&0xffffffff==16)
    tpoff=next(r for r in reloc if r[2]&0xffffffff==18)
    mutate('module-addend',dtp[0]+16,'q',1)
    mutate('overflow-offset',tpoff[0]+16,'q',2**63-1)
    mutate('metadata-target',dtp[0],'Q',tags[7])
    relro_index=next(i for i,p in enumerate(ph) if p[0]==0x6474e552)
    mutate('relro-unmapped-tail',phoff+relro_index*56+40,'Q',ph[relro_index][6]+8192)
    desc=bytearray((out/'desc0.so').read_bytes())
    d_ph=[struct.unpack_from('<IIQQQQQQ',desc,struct.unpack_from('<Q',desc,32)[0]+i*56)
          for i in range(struct.unpack_from('<H',desc,56)[0])]
    d_dynamic=next(p for p in d_ph if p[0]==2);d_tags={}
    for off in range(d_dynamic[2],d_dynamic[2]+d_dynamic[5],16):
        tag,value=struct.unpack_from('<qQ',desc,off)
        if not tag:break
        d_tags[tag]=value
    def d_fileoff(va):
        p=next(p for p in d_ph if p[0]==1 and p[3]<=va<p[3]+p[5]);return p[2]+va-p[3]
    for start,size in ((d_tags[7],d_tags[8]),(d_tags[23],d_tags[2])):
        for off in range(d_fileoff(start),d_fileoff(start)+size,24):
            _,info,_=struct.unpack_from('<QQq',desc,off)
            if info&0xffffffff==36:
                struct.pack_into('<Q',desc,off,d_dynamic[3]-8);break
        else:continue
        break
    else:raise AssertionError('missing actual TLSDESC relocation')
    variants['descriptor-tail-metadata']=desc;invalid.append('descriptor-tail-metadata')
    data=bytearray(original);struct.pack_into('<Q',data,pos+48,4096)
    variants['aligned-template']=data # exercises nonzero first-byte alignment
    data=bytearray(original);struct.pack_into('<Q',data,40,0);struct.pack_into('<HHH',data,58,0,0,0)
    variants['sectionless']=data
    for name,data in variants.items():
        image=out/(name+'.so');image.write_bytes(data);specs.append(f'{name}={image}')
    # Include TLS mapping allocation in a real low-RAM failed-load rollback.
    data=bytearray(original);last=max(i for i,p in enumerate(ph) if p[0]==1)
    struct.pack_into('<Q',data,phoff+last*56+40,32*1024*1024)
    image=out/'oom.so';image.write_bytes(data);specs.append(f'oom={image}')
    disk=out/'assets.img';disk.unlink(missing_ok=True);create(disk,specs)
    return disk,invalid,unsupported


def main(uefi=False):
    disk,invalid,unsupported=fixtures();options={'ram':16}
    if uefi:options=dict(ram=128,boot_image=Path('build/myos-uefi.img'),boot_bus='ide',
                        firmware=(Path('/usr/share/OVMF/OVMF_CODE_4M.fd'),Path('/usr/share/OVMF/OVMF_VARS_4M.fd')))
    vm=VM('native-tls-uefi' if uefi else 'native-tls',asset_disk=disk,**options)
    try:
        vm.wait('myos> ',30)
        for name in invalid+unsupported:
            output=long_command(vm,'elf live '+name)
            assert ('unsupported runtime feature' if name in unsupported else 'error: invalid executable') in output,output
            assert 'native TLS constructor' not in output and 'USER FAULT' not in output,output
        output=long_command(vm,'elf live zero')
        assert 'native TLS zero-only/import-free PASS' in output and 'ELF exited with status 59' in output,output
        assert_live_resources(output)
        for name in ('gd0','desc0','ie0','ld0','aligned-template','sectionless','gd0'):
            output=long_command(vm,'elf live '+name)
            assert 'ELF exited with status 58' in output and 'native ELF TLS template/zero/relocation' in output,output
            assert 'native TLS constructor PASS' in output and 'native TLS finalizer PASS' in output,output
            assert_live_resources(output)
        for name,vector in (('gd1',6),('gd2',14),('gd3',14)):
            output=long_command(vm,'elf live '+name)
            assert 'USER FAULT' in output and f'vector={vector} ' in output and 'native TLS finalizer PASS' not in output,output
            if name=='gd2':assert 'error=0x0000000000000015' in output,output
            if name=='gd3':
                import re
                workers=int(re.search(r'MYOS_WORKER_LIMIT\s+(\d+)u',Path('include/thread-limit.h').read_text()).group(1))
                guard=0x803f010000+(workers+1)*4096
                assert 'error=0x0000000000000006' in output and f'address=0x{guard:016X}' in output,output
            assert_live_resources(output)
        cancel_live(vm,'gd4','native TLS blocked cancel ready',timeout=15)
        if not uefi:
            output=long_command(vm,'elf live oom');assert 'error: out of memory' in output,output
        output=long_command(vm,'elf live desc0');assert 'ELF exited with status 58' in output,output
        assert_live_resources(output)
        print('PASS: native x64 ELF TLS GD/IE/TLSDESC, relocated templates/TBSS, constructor/finalizer, '
              'FS/errno isolation, pthread destructors, join/detach slot reuse, malformed bounds, '
              'NX/guard/worker faults, cancellation/relaunch and exact page/heap reclamation. '
              'NOT a renderer or Cobalt acceptance pass.')
    finally:vm.close()


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--uefi',action='store_true')
    main(parser.parse_args().uefi)
