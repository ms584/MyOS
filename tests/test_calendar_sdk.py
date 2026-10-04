"""Real UTC calendar and musl strftime, including genuine GCC C++ ctime headers."""
from pathlib import Path
import argparse
import calendar
import contextlib
import datetime
import runpy
import shutil
import subprocess
import sys
root=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(root/'tests'));sys.path.insert(0,str(root/'tools'))
from test_c11_sdk import digest
from test_system import VM
from test_elf_dynamic import long_command,assert_live_resources
from make_asset_disk import create
parser=argparse.ArgumentParser();parser.add_argument('--configuration',type=Path,required=True)
parser.add_argument('--uefi',action='store_true');args=parser.parse_args()
cfg=args.configuration.resolve();assert cfg.is_relative_to((root/'build/cxx-target-evaluation').resolve())
base=root/'build/time-evaluation';base.mkdir(parents=True,exist_ok=True)
with (base/'last-calendar-build.log').open('w') as log,contextlib.redirect_stdout(log):
    build=runpy.run_path(str(root/'tools/build_time_sdk.py'));out=build['out']
    formatting=runpy.run_path(str(root/'tools/build_format_sdk.py'))
    scan=runpy.run_path(str(root/'tools/build_scan_sdk.py'),init_globals={'math_build':formatting['math']})
epoch=datetime.datetime(1970,1,1);rows=[]
for year in (1600,1900,1969,1970,2000,2001,2038,2099,2100,2400,9999):
    for month in range(1,13):
        for day in (1,calendar.monthrange(year,month)[1]):
            d=datetime.datetime(year,month,day,13,27,39);delta=d-epoch
            seconds=delta.days*86400+delta.seconds;iso=d.isocalendar()
            yday=(d.date()-datetime.date(year,1,1)).days+1
            text=f'{year:04}-{month:02}-{day:02} 13:27:39|{yday:03}|{(d.weekday()+1)%7}|{iso.year:04}-{iso.week:02}-{iso.weekday}|UTC|+0000'
            rows.append('{'+f'{seconds}LL,{year},{month},{day},"{text}"'+'}')
(out/'calendar-vectors.h').write_text('struct calendar_vector {time_t seconds;int year,month,day;const char *formatted;};\n'
    +'static const struct calendar_vector vectors[]={\n'+',\n'.join(rows)+'\n};\n')
assets=[]
for language in ('c','cpp98','cpp17'):
    obj=out/(language+'.o');library=out/(language+'.so')
    flags=build['flags']+['-I'+str(out)]
    if language!='c':
        flags=[f for f in flags if f not in ('-std=c11','-ffreestanding')]+[
            '-std=c++98' if language=='cpp98' else '-std=c++17','-nostdinc++','-fno-exceptions','-fno-rtti']
        flags=['-isystem',str(cfg/'build/include'),'-isystem',str(cfg/'build/include/x86_64-unknown-myos'),*flags]
    subprocess.run(['gcc' if language=='c' else 'g++',*flags,'-x','c' if language=='c' else 'c++',
                    '-c',str(root/'user/elf-calendar.c'),'-o',str(obj)],check=True)
    subprocess.run(['ld','-shared','--hash-style=both','-Bsymbolic','-z','now','-z','relro',
                    '-z','noexecstack','-e','_start','-o',str(library),str(obj),str(build['library']),
                    str(formatting['library']),str(scan['library']),str(formatting['math']['static_library'])],check=True)
    assert '(NEEDED)' not in subprocess.check_output(['readelf','-dW',str(library)],text=True)
    assets.append(language+'='+str(library))
disk=out/'assets.img';create(disk,assets)
boot=root/'build'/('myos-uefi.img' if args.uefi else 'myos-current.img')
before=digest(boot);image=out/'frozen-boot.img';shutil.copyfile(boot,image)
options=dict(ram=64,boot_image=image,asset_disk=disk)
if args.uefi:options.update(ram=128,boot_bus='ide',firmware=(Path('/usr/share/OVMF/OVMF_CODE_4M.fd'),Path('/usr/share/OVMF/OVMF_VARS_4M.fd')))
vm=VM('calendar-uefi' if args.uefi else 'calendar',**options)
try:
    vm.wait('myos> ')
    for _ in range(2):
        for name in ('c','cpp98','cpp17'):
            result=long_command(vm,'elf live '+name,timeout=60)
            assert 'ELF exited with status 101' in result and 'UTC calendar/strftime/bounds/8workers PASS' in result,result
            assert 'USER FAULT' not in result and 'USER SIGNAL' not in result,result
            assert_live_resources(result)
    print('PASS: native UTC',len(rows),'civil/ISO-week oracle vectors, normalization/limits/'
          'strftime/errno/guards/eight workers, real GCC C++98/17 ctime, six relaunches/'
          'exact resources; CPU clock honestly unavailable, NOT timezone/locale switching/full C++/Cobalt.',out,flush=True)
finally:vm.close()
assert digest(boot)==before==digest(image)
