"""Native pthread teardown/timed waits; not Cobalt or complete POSIX acceptance."""
import argparse
from pathlib import Path
import subprocess
from test_system import VM,OUT
from test_elf_dynamic import long_command,cancel_live,assert_live_resources
from make_asset_disk import create


def fixtures():
    out=OUT/'thread-services';out.mkdir(exist_ok=True);specs=[]
    for name,source,defines,fini in (
        ('exit','exit',[],True),('detach-main','exit',['-DDETACH_MAIN=1'],True),
        ('timed','timed',[],False),('cancel','timed',['-DCANCEL_WAIT=1'],False),
        ('claim','claim',[],False)):
        obj=out/(name+'.o');image=out/(name+'.so')
        subprocess.run(['gcc','-m64','-std=c11','-Os','-fPIC','-ffreestanding','-fno-builtin',
                        '-mno-red-zone','-fno-stack-protector','-fno-asynchronous-unwind-tables',
                        '-fno-unwind-tables','-Wall','-Wextra','-Werror',*defines,
                        '-c',f'user/elf-thread-{source}.c','-o',str(obj)],check=True)
        subprocess.run(['ld','-shared','--hash-style=both','-z','now','-z','relro','-e','_start',
                        *(['-fini','finalizer'] if fini else []),'-o',str(image),str(obj)],check=True)
        assert '(NEEDED)' not in subprocess.check_output(['readelf','-dW',str(image)],text=True)
        specs.append(f'{name}={image}')
    disk=out/'assets.img';disk.unlink(missing_ok=True);create(disk,specs);return disk


def main(uefi=False):
    options={}
    if uefi:options=dict(ram=128,boot_image=Path('build/myos-uefi.img'),boot_bus='ide',
                        firmware=(Path('/usr/share/OVMF/OVMF_CODE_4M.fd'),Path('/usr/share/OVMF/OVMF_VARS_4M.fd')))
    vm=VM('thread-services-uefi' if uefi else 'thread-services',asset_disk=fixtures(),**options)
    try:
        vm.wait('myos> ')
        markers={'exit':'join/destructors/finalizer PASS','detach-main':'join/destructors/finalizer PASS',
                 'timed':'realtime/overflow/contention/errno PASS','claim':'exclusive ownership PASS'}
        for _ in range(3):
            for name,marker in markers.items():
                output=long_command(vm,'elf live '+name)
                assert 'ELF exited with status 0' in output and marker in output,output
                assert_live_resources(output)
        cancel_live(vm,'cancel','native timed mutex cancel ready',timeout=15)
        output=long_command(vm,'elf live exit')
        assert 'ELF exited with status 0' in output and markers['exit'] in output,output
        assert_live_resources(output)
        print('PASS: native main/worker pthread_exit, main join/detach, repeated TSS destructors/finalizers, '
              'realtime timed mutex/overflow/errors, exclusive join/detach races, cancellation/relaunch/resources. '
              'NOT complete POSIX or Cobalt acceptance.',flush=True)
    finally:vm.close()


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--uefi',action='store_true')
    main(parser.parse_args().uefi)
