"""Public YouTube native Cobalt feasibility observation, never automatic acceptance.
No host page/media fetching, fixture CA, proxy, decoder, or credentials.
"""
from pathlib import Path
import argparse, datetime, hashlib, json, os, re, shutil, subprocess, sys, tempfile, time, wave
from PIL import Image
from test_system import ROOT, VM
from test_shell import command, type_text
sys.path.insert(0, str(ROOT/'tools'))
from make_asset_disk import create

def digest(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()

def native_click(vm,x,y,button='left'):
    assert button in ('left','right')
    # Clamp the actual PS/2 cursor to the top-left, then
    # move in bounded deltas; never read DOM/guest heaps.
    for _ in range(8):
        vm.command('input-send-event',{'events':[{'type':'rel','data':{'axis':'x','value':-120}},{'type':'rel','data':{'axis':'y','value':-100}}]})
        time.sleep(.03)
    for axis,value in [('x',x),('y',y)]:
        while value:
            step=min(value,100);value-=step
            vm.command('input-send-event',{'events':[{'type':'rel','data':{'axis':axis,'value':step}}]});time.sleep(.03)
    time.sleep(.2)
    for down in (True,False):
        vm.command('input-send-event',{'events':[{'type':'btn','data':{'down':down,'button':button}}]});time.sleep(.1)

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('resources',type=Path);p.add_argument('fonts',type=Path)
    p.add_argument('--provider',type=Path,default=ROOT/'build/media-evaluation/provider-i8xbwfmz')
    p.add_argument('--url',action='append',required=True)
    p.add_argument('--engine-trace',action='store_true',help='Native opt-in engine stderr/VLOG only; no memory extraction or TLS overrides')
    p.add_argument('--identity',type=Path,default=ROOT/'docs/wip/phase333/checkpoint6/final-normal-identity.json')
    p.add_argument('--memory-mib',type=int,choices=(512,1024),default=512,help='Actual QEMU physical RAM; native E820/PMM, no invented memory')
    p.add_argument('--seconds',type=int,default=300)
    p.add_argument('--input',action='append',default=[],help='Timed native key: seconds:qcode')
    p.add_argument('--actions',type=Path,help='Optional local JSON native key/text/click actions; no page scripting')
    p.add_argument('--desktop-check',action='store_true',help='Native desktop window/input smoke after each player close')
    p.add_argument('--ram',action='store_true',help='Read-only sampled PMM; includes brief QMP pauses')
    a=p.parse_args()
    assert 30<=a.seconds<=600
    assert all(u.startswith('https://www.youtube.com/') and not any(c.isspace() for c in u) for u in a.url)
    assert len(a.url)<=4
    keys=[]
    for item in a.input:
        at,key=item.split(':',1);at=float(at)
        assert 0<=at<a.seconds and key in ('spc','tab','ret','left','right','up','down','pgdn','pgup','k','j','l','f','m')
        keys.append((at,key))
    os.umask(0o077)
    out=Path(tempfile.mkdtemp(prefix='phase334-public-',dir=ROOT/'build'))
    audit=json.loads((ROOT/'docs/PHASE33_COBALT_AUDIT.json').read_text())
    core=ROOT/'build/engine-evaluation/libcobalt.so';provider=a.provider.resolve()/'provider.so'
    assert digest(core)==audit['library_sha256']
    assert digest(provider)==json.loads((provider.parent/'report.json').read_text())['provider_sha256']
    specs=['cobalt='+str(core),'provider='+str(provider)]
    report=json.loads((a.resources/'report.json').read_text())
    assert report['package_sha256']==audit['package_sha256']
    for item in report['resources']:
        source=a.resources/'content'/item['name']
        assert source.stat().st_size==item['bytes'] and digest(source)==item['sha256']
        specs.append(item['name']+'='+str(source))
    for item in json.loads((a.fonts/'report.json').read_text())['resources']:
        source=a.fonts/item['name']
        assert source.stat().st_size==item['bytes'] and digest(source)==item['sha256']
        specs.append('systemfonts/'+item['name']+'='+str(source))
    identities=json.loads(a.identity.read_text())
    for name,expected in identities.items():
        assert digest(ROOT/'build'/name)==expected, 'Normal prerequisite identity mismatch: '+name
    boot=out/'boot.img';runtime=out/'platform-runtime.mrt'
    shutil.copyfile(ROOT/'build/myos-current.img',boot)
    shutil.copyfile(ROOT/'build/platform-runtime.mrt',runtime)
    disk=out/'assets.img';create(disk,specs,runtime=runtime)
    sys.path.insert(0,str(ROOT/'docs/wip/native-pmm512/kvm-ata-20261008'))
    from qemu_kvm_optin_20261008 import install
    original=subprocess.Popen
    qemu=[]
    def capture(args,*pos,**kw):
        if isinstance(args,list) and args and Path(args[0]).name=='qemu-system-x86_64':
            args=list(args);i=args.index('-audiodev')+1
            args[i]+=',out.frequency=11025,out.channels=1,out.format=u8'
            qemu.append(args)
        return original(args,*pos,**kw)
    subprocess.Popen=capture
    install()
    evidence=dict(phase_passed=False,observation_only=True,date=datetime.datetime.now(datetime.timezone(datetime.timedelta(hours=7))).isoformat(),
        commit=subprocess.check_output(['git','rev-parse','HEAD'],text=True).strip(),
        core_sha256=digest(core),provider_sha256=digest(provider),normal_identity=identities,
        urls=a.url,qemu=qemu,launches=[],memory_mib=a.memory_mib,engine_trace=a.engine_trace,diagnostic_provider=json.loads((provider.parent/'report.json').read_text()).get('diagnostic_trace',False),host_fetch=False,fixture_ca_added=False)
    vm=None;observer=None
    try:
        vm=VM('phase334-public',ram=a.memory_mib,nic=True,asset_disk=disk,boot_image=boot,audio=out/'audio.wav',cpu='max')
        if a.ram:
            from cobalt_pmm_observer import Observer
            observer=Observer(vm)
        vm.wait('myos> ');command(vm,'net up','Network: DHCP configured');command(vm,'mkdir /dev/shm','myos> ')
        for n,url in enumerate(a.url,1):
            row=dict(launch=n,url=url,frames=[],inputs=[],before=command(vm,'mem','Free physical pages: '))
            evidence['launches'].append(row)
            offset=len(vm.log.read_text());started=time.monotonic()
            type_text(vm,('elf cobalt-trace' if a.engine_trace else 'elf cobalt')+' cobalt provider '+url+'\n')
            next_frame=0;sent=set();action_sent=set()
            if observer:observer.capture(n,0,'before')
            while time.monotonic()-started<a.seconds:
                elapsed=time.monotonic()-started;content=vm.log.read_text()[offset:]
                if elapsed>=next_frame:
                    next_frame=elapsed+5
                    if observer:observer.capture(n,elapsed)
                    ppm=out/('launch-%d-%03d.ppm'%(n,elapsed))
                    vm.command('screendump',{'filename':str(ppm)})
                    png=ppm.with_suffix('.png');Image.open(ppm).save(png);ppm.unlink()
                    row['frames'].append(dict(elapsed=elapsed,path=png.name))
                for index,(at,key) in enumerate(keys):
                    if elapsed>=at and index not in sent:
                        vm.key(key);sent.add(index);row['inputs'].append(dict(key=key,elapsed=elapsed))
                if a.actions and a.actions.exists():
                    actions=json.loads(a.actions.read_text())
                    assert isinstance(actions,list) and len(actions)<=40
                    for action in actions:
                        if 'launch' in action and int(action['launch'])!=n:continue
                        ident=str(action['id'])
                        if ident in action_sent or elapsed<float(action.get('at',0)):continue
                        kind=action['type']
                        if kind=='click':
                            x,y=int(action['x']),int(action['y'])
                            assert 0<=x<800 and 0<=y<600
                            native_click(vm,x,y,action.get('button','left'))
                        elif kind=='text':
                            text=action['text'];assert isinstance(text,str) and len(text)<=64 and all(ord(c)<128 for c in text)
                            type_text(vm,text)
                        elif kind=='key':
                            assert action['key'] in ('spc','tab','ret','left','right','up','down','k','j','l','f','m')
                            vm.key(action['key'])
                        elif kind=='link':
                            assert isinstance(action['up'],bool)
                            network=vm.command('human-monitor-command',{'command-line':'info network'})
                            names=re.findall(r'^([^ :\n]+):.*type=nic',network,re.M)
                            assert len(names)==1, 'Expected one isolated VM NIC'
                            vm.command('set_link',{'name':names[0],'up':action['up']})
                        elif kind=='close':
                            vm.key('esc')
                        else:raise ValueError('Unsupported native action')
                        action_sent.add(ident);row['inputs'].append(dict(action=action,elapsed=elapsed))
                if 'USER FAULT' in content or 'PANIC' in content or content.endswith('myos> '):break
                time.sleep(.1)
            row['elapsed']=time.monotonic()-started
            if not vm.log.read_text().endswith('myos> '):
                vm.key('esc');row['inputs'].append(dict(key='esc',elapsed=row['elapsed']))
                end=time.monotonic()+20
                while time.monotonic()<end and not vm.log.read_text().endswith('myos> '):time.sleep(.1)
            row['shell_returned']=vm.log.read_text().endswith('myos> ')
            if row['shell_returned']:
                row['after']=command(vm,'mem','Free physical pages: ')
                if a.desktop_check:
                    desktop_offset=len(vm.log.read_text())
                    type_text(vm,'desktop'+chr(10))
                    end=time.monotonic()+30
                    while 'DESKTOP READY' not in vm.log.read_text()[desktop_offset:] and time.monotonic()<end:time.sleep(.1)
                    assert 'DESKTOP READY' in vm.log.read_text()[desktop_offset:], 'Desktop did not start after player close'
                    native_click(vm,80,105)
                    presented='WINDOW OPEN 1'+chr(10)+'DESKTOP FRAME'+chr(10)
                    end=time.monotonic()+10
                    while presented not in vm.log.read_text()[desktop_offset:] and time.monotonic()<end:time.sleep(.1)
                    assert presented in vm.log.read_text()[desktop_offset:], 'Desktop input did not present a window after player close'
                    ppm=out/('desktop-after-launch-%d.ppm'%n)
                    vm.command('screendump',{'filename':str(ppm)})
                    png=ppm.with_suffix('.png');Image.open(ppm).save(png);ppm.unlink()
                    vm.key('q')
                    end=time.monotonic()+10
                    while not vm.log.read_text().endswith('myos> ') and time.monotonic()<end:time.sleep(.1)
                    assert vm.log.read_text().endswith('myos> '), 'Desktop did not return to shell'
                    row['desktop_after_close']=dict(window_opened=True,shell_returned=True,path=png.name)
                    row['after_desktop']=command(vm,'mem','Free physical pages: ')

                if observer:observer.capture(n,time.monotonic()-started,'after')
            (out/'evidence.json').write_text(json.dumps(evidence,indent=2)+'\n')
            print('PUBLIC_OBSERVATION',json.dumps({k:v for k,v in row.items() if k!='frames'}),flush=True)
            assert row['shell_returned'], 'Cannot safely reopen without shell recovery'
    finally:
        if vm:
            (out/'serial.log').write_text(vm.log.read_text());vm.close()
        evidence['qemu']=qemu
        if observer:evidence['pmm']=observer.report()
        evidence['normal_identity_after']={name:digest(ROOT/'build'/name) for name in identities}
        assert evidence['normal_identity_after']==identities
        (out/'evidence.json').write_text(json.dumps(evidence,indent=2)+'\n')
        print('EVIDENCE_BUNDLE',out,'OBSERVATION ONLY; Phase33.4 NOT PASSED',flush=True)

if __name__=='__main__':main()

