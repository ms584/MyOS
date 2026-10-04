"""Two-image native lifecycle and separate genuine provider; never Cobalt proof."""
import argparse,hashlib,json,re,shutil,struct,subprocess,sys,tempfile,time
from pathlib import Path
from test_system import VM,ROOT
from test_shell import type_text
from test_elf_dynamic import long_command
sys.path.insert(0,str(ROOT/'tools'))
from make_asset_disk import create
p=argparse.ArgumentParser(description=__doc__)
p.add_argument('--provider',type=Path,required=True)
p.add_argument('--uefi',action='store_true')
p.add_argument('--ram',type=int,choices=(64,128,256),default=256)
args=p.parse_args()
provider=args.provider.resolve();report=json.loads((provider/'report.json').read_text())
digest=lambda path:hashlib.sha256(path.read_bytes()).hexdigest()
assert digest(provider/'provider.so')==report['provider_sha256'] and report['paired_exit_abi']==1
core=ROOT/'build/engine-evaluation/libcobalt.so'
assert digest(core)==json.loads((ROOT/'docs/PHASE33_COBALT_AUDIT.json').read_text())['library_sha256']
pins=json.loads((ROOT/'docs/PHASE331_ABI_SOURCES.json').read_text())['headers']
for name in ('egl.h','gles.h'):
    expected=next(item['sha256'] for item in pins if item['path']=='starboard/'+name)
    assert digest(ROOT/'build/egl-core-evaluation/core-0ky0zq6u/include/starboard'/name)==expected
out=Path(tempfile.mkdtemp(prefix='pair-exec-',dir=ROOT/'build/renderer-provider-evaluation'))
intrinsics=subprocess.check_output(['gcc','-print-file-name=include'],text=True).strip()
flags=['-m64','-std=c11','-Os','-fPIC','-ffreestanding','-fno-builtin','-mno-red-zone',
       '-fno-stack-protector','-fno-asynchronous-unwind-tables','-fno-unwind-tables',
       '-nostdinc','-isystem',intrinsics,'-I'+str(ROOT/'user/sdk'),
       '-I'+str(ROOT/'build/egl-core-evaluation/core-0ky0zq6u/include'),
       '-I'+str(ROOT/'build/renderer-evaluation/target-config-snob6k4x/mesa-25.0.7/include'),
       '-I'+str(ROOT/'build/native-renderer-evaluation/native-21xnvj1n/include')]
exitobj=out/'exit.o'
subprocess.run(['gcc',*flags,'-DMYOS_PAIRED_IMAGE=1','-c',str(ROOT/'user/platform-exit.c'),'-o',str(exitobj)],check=True)
finalize=out/'finalize.o'
subprocess.run(['gcc',*flags,'-c',str(ROOT/'user/platform-cxx-finalize.c'),'-o',str(finalize)],check=True)
assets=['actual='+str(provider/'provider.so')]
for name,defines in [('normal',[]),('worker',['WORKER_EXIT']),('recursive',['RECURSIVE_EXIT']),
                     ('raw',['RAW_EXIT']),('loop',['LOOP']),('render',['ACTUAL_RENDERER']),
                     ('concurrent',['CONCURRENT_EXIT']),('fault',['FAULT_ENTRY']),
                     ('controlnx',['CONTROL_EXEC']),
                     ('faultfini',['FAULT_FINI']),('loopfini',['LOOP_FINI']),
                     ('provider',['PROVIDER']),('ctor',['PROVIDER','CTOR_EXIT'])]:
    obj=out/(name+'.o');lib=out/(name+'.so')
    subprocess.run(['gcc',*flags,*['-D'+d+'=1' for d in defines],'-c',str(ROOT/'user/elf-pair-fixture.c'),'-o',str(obj)],check=True)
    subprocess.run(['ld','-shared','--hash-style=both','-Bsymbolic','-z','now','-z','relro',
        '-z','noexecstack','-e','_start','-o',str(lib),str(obj),str(exitobj),str(finalize)],check=True)
    assert '(NEEDED)' not in subprocess.check_output(['readelf','-dW',str(lib)],text=True)
    assets.append(name+'='+str(lib))
old=ROOT/'build/renderer-provider-evaluation/provider-njs6506e/provider.so'
assets.extend(['old='+str(old),'cobalt='+str(core)])
if args.ram<256:
    # Bounded real-provider LOAD mutation: test failure while mapping image1,
    # not a fabricated renderer or a claim about Cobalt's required RAM.
    data=bytearray((provider/'provider.so').read_bytes())
    phoff=struct.unpack_from('<Q',data,32)[0];count=struct.unpack_from('<H',data,56)[0]
    loads=[(phoff+i*56,struct.unpack_from('<IIQQQQQQ',data,phoff+i*56)) for i in range(count)]
    last=max((item for item in loads if item[1][0]==1),key=lambda item:item[1][3])
    struct.pack_into('<Q',data,last[0]+40,last[1][6]+96*1024*1024)
    wide=out/'oom-provider.so';wide.write_bytes(data);assets.append('oom='+str(wide))
disk=out/'assets.img';create(disk,assets)
boot=ROOT/'build'/('myos-uefi.img' if args.uefi else 'myos-current.img')
before=digest(boot);frozen=out/'frozen-boot.img';shutil.copyfile(boot,frozen)
options=dict(ram=args.ram,boot_image=frozen,asset_disk=disk)
if args.uefi:options.update(boot_bus='ide',firmware=(Path('/usr/share/OVMF/OVMF_CODE_4M.fd'),Path('/usr/share/OVMF/OVMF_VARS_4M.fd')))
vm=VM('elf-pair-execution'+('-uefi' if args.uefi else ''),**options)
def resources(output):
    m=re.search(r'ELF pair live: pages reclaimed; heap-before=(\d+) heap-after=(\d+)',output)
    assert m and m[1]==m[2],output
def ordered(output,markers):
    positions=[output.index(m) for m in markers];assert positions==sorted(positions),output
    for marker in markers:assert output.count(marker)==1,output
try:
    vm.wait('myos> ')
    for repeat in range(2):
        for name in ('normal','worker','recursive','raw','concurrent'):
            output=long_command(vm,'elf pair-live '+name+' provider')
            assert 'ELF exited with status '+str(78 if name=='recursive' else 77) in output,output
            markers=['pair provider init','pair core init','pair core entry']
            if name!='raw':markers+=['pair core cxa','pair core fini','pair provider cxa','pair provider fini']
            else:assert ' cxa' not in output and ' fini' not in output,output
            ordered(output,markers);resources(output)
        output=long_command(vm,'elf pair-live normal ctor')
        ordered(output,['pair provider init','pair provider cxa','pair provider fini'])
        assert 'pair core init' not in output and 'pair core cxa' not in output,output
        assert 'ELF exited with status 77' in output,output;resources(output)
        output=long_command(vm,'elf pair-live render actual')
        assert output.count('pair genuine EGL/GLES pixel/TLS PASS')==2 and 'ELF exited with status 77' in output,output
        resources(output)
        for name in ('fault','faultfini','controlnx'):
            output=long_command(vm,'elf pair-live '+name+' provider')
            assert 'USER FAULT' in output and 'pair provider cxa' not in output,output
            assert output.count('pair core fini')==(1 if name=='faultfini' else 0),output
            if name=='controlnx':
                fault=re.search(r'USER FAULT pid=\d+ vector=14 error=(0x[0-9a-fA-F]+) rip=0x0000008010011000 address=0x0000008010011000',output)
                assert fault and int(fault[1],16)&0x15==0x15,output
            resources(output)
    for cmd in ('elf pair-live render old','elf pair-starboard normal provider'):
        output=long_command(vm,cmd)
        assert 'pair provider init' not in output and 'pair core init' not in output,output
        assert 'unsupported runtime feature' in output or ('Starboard SABI: mismatch; constructors not executed' in output and 'ELF exited with status 95' in output),output
    output=long_command(vm,'elf pair-starboard cobalt actual')
    if args.ram==256:assert 'ELF bindings: missing=55' in output and 'unsupported runtime feature' in output,output
    else:
        assert 'out of memory' in output.lower(),output
        output=long_command(vm,'elf pair-live normal oom')
        assert 'out of memory' in output.lower() and 'ELF pair mapping module: 1' in output,output
        assert 'pair core init' not in output,output
    assert 'ELF pair live: started' not in output,output
    for name,marker in [('loop','pair waiting ready'),('loopfini','pair looping finalizer ready')]:
        offset=len(vm.log.read_text());type_text(vm,'elf pair-live '+name+' provider\n')
        deadline=time.monotonic()+90
        while time.monotonic()<deadline:
            output=vm.log.read_text()[offset:]
            if marker in output:break
            assert 'PANIC' not in output,output
            time.sleep(.1)
        else:raise AssertionError(output)
        vm.command('input-send-event',{'events':[{'type':'key','data':{'down':down,'key':{'type':'qcode','data':'esc'}}} for down in (True,False)]})
        deadline=time.monotonic()+10
        while time.monotonic()<deadline:
            output=vm.log.read_text()[offset:]
            if output.endswith('myos> '):break
            time.sleep(.1)
        assert 'ELF cancelled; all threads stopped' in output and 'pair provider cxa' not in output and 'pair provider fini' not in output,output
        if name=='loop':assert ' cxa' not in output and ' fini' not in output,output
        resources(output)
    assert 'App exited with status 42' in long_command(vm,'run /bin/hello')
    print('PASS: pair genuine worker EGL/GLES pixels/TLS, dependency init/reverse fini, worker/concurrent8/recursive/constructor/raw exit, fault/fini-fault/fini-loop, SABI-before-init/old-ABI/missing-core rejection, cancellation/relaunch/exact resources; NOT Cobalt.',out,flush=True)
finally:
    vm.close();shutil.copyfile(vm.log,out/'serial.log');assert digest(boot)==before
