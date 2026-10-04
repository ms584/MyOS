"""Real Mesa OSMesa in MyOS with installed runtime, pixels/input/resource checks."""
import argparse
import contextlib
import hashlib
import json
from pathlib import Path
import runpy
import re
import shutil
import subprocess
import sys
import tempfile
import time
root=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(root/'tools'));sys.path.insert(0,str(root/'tests'))
from build_cxx_containers_sdk import build
from make_asset_disk import create
from test_system import VM
from test_shell import type_text
from test_elf_dynamic import assert_live_resources
from test_video import frame_shot
parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--renderer-probe',type=Path,required=True)
parser.add_argument('--configuration',type=Path,required=True)
parser.add_argument('--uefi',action='store_true')
parser.add_argument('--gles',action='store_true',help='Real Mesa GLES profile/GLSL100, not EGL')
parser.add_argument('--egl-core',type=Path,help='Actual Mesa EGL core + MyOS pbuffer driver archive')
parser.add_argument('--egl-window',action='store_true',help='Present through an actual native EGL window swap')
parser.add_argument('--diagnostic',action='store_true',help='Transparent test-only allocation/lock wrappers')
parser.add_argument('--launches',type=int,choices=(1,2,3),default=2)
parser.add_argument('--ram',type=int,choices=(128,256,512,1024),default=256)
parser.add_argument('--startup-timeout',type=int,choices=(120,300,600),default=120,
                    help='Bound cold Mesa shader initialization; never a Cobalt pass criterion')
args=parser.parse_args()
if args.egl_core and not args.gles:parser.error('--egl-core requires --gles')
if args.egl_window and not args.egl_core:parser.error('--egl-window requires --egl-core')
probe=args.renderer_probe.resolve();cfg=args.configuration.resolve()
assert probe.is_relative_to((root/'build/renderer-evaluation').resolve())
assert cfg.is_relative_to((root/'build/cxx-target-evaluation').resolve())
report=json.loads((probe/'incremental-report.json').read_text())
assert report['static_renderer_link_passed'] and not report['host_libraries_allowed']
base=root/'build/native-renderer-evaluation';base.mkdir(parents=True,exist_ok=True)
out=Path(tempfile.mkdtemp(prefix='native-',dir=base))
with (out/'sdk-build.log').open('w') as log,contextlib.redirect_stdout(log):
    math=runpy.run_path(str(root/'tools/build_math_sdk.py'))
    sdk=[runpy.run_path(str(root/'tools'/name),init_globals={'math_build':math})
         for name in ('build_format_sdk.py','build_sort_sdk.py','build_scan_sdk.py',
                      'build_time_sdk.py','build_jump_sdk.py')]
    containers=build(cfg,probe/'libmyos-runtime-probe.a')
headers=out/'include/starboard';headers.mkdir(parents=True)
(headers/'export.h').write_text('#define SB_EXPORT\n')
pin=json.loads((root/'docs/PHASE331_ABI_SOURCES.json').read_text())
window=root/'build/engine-evaluation/contracts/window.h'
assert hashlib.sha256(window.read_bytes()).hexdigest()==next(
    item['sha256'] for item in pin['headers'] if item['path'].endswith('/window.h'))
intrinsics=subprocess.check_output(['gcc','-print-file-name=include'],text=True).strip()
flags=['-m64','-Os','-fPIC','-ffreestanding','-fno-builtin','-mno-red-zone',
       '-D__myos__=1',*(['-DMYOS_NATIVE_GLES=1'] if args.gles else []),
       *(['-DMYOS_NATIVE_EGL=1','-DEGL_NO_X11=1'] if args.egl_core else []),
       *(['-DMYOS_NATIVE_EGL_WINDOW=1'] if args.egl_window else []),
       '-fno-stack-protector','-fno-asynchronous-unwind-tables','-fno-unwind-tables',
       '-nostdinc','-isystem',intrinsics,'-I'+str(root/'user/sdk'),
       '-I'+str(root/'include'),'-I'+str(window.parent),'-I'+str(out/'include'),
       '-I'+str(probe/'mesa-25.0.7/include'),'-Wall','-Wextra','-Werror']
fixture=out/'fixture.o';finalize=out/'finalize.o'
font=root/'assets/myos-font.bin'
assert hashlib.sha256(font.read_bytes()).hexdigest()==(
    '38cea9db0e28a9458cad63a57050167cbb87c94f2f5805d097f7cfee871bda94')
font_object=out/'font.o'
subprocess.run(['nasm','-f','elf64',str(root/'user/renderer-font.asm'),
                '-o',str(font_object)],cwd=root,check=True)
environment=out/'environment.o';process_exit=out/'process-exit.o'
for source,target in [(root/'user/elf-native-renderer.c',fixture),
                      (root/'user/platform-cxx-finalize.c',finalize),
                      (root/'user/platform-environment.c',environment),
                      (root/'user/platform-exit.c',process_exit)]:
    subprocess.run(['gcc','-std=c11',*flags,'-c',str(source),'-o',str(target)],check=True)
trace=[]
if args.diagnostic:
    obj=out/'trace.o'
    subprocess.run(['gcc','-std=c11',*flags,'-c',str(root/'user/renderer-trace.c'),'-o',str(obj)],check=True)
    trace=[str(obj),*['--wrap='+name for name in ('malloc','calloc','pthread_mutex_lock',
        'pthread_once','__cxa_guard_acquire','__cxa_guard_release')]]
mesa=probe/'build'
archives=[mesa/path for path in (
    'src/gallium/frontends/osmesa/libosmesa_st.a','src/mapi/glapi/libglapi_bridge.a',
    'src/mesa/libmesa.a','src/mesa/libmesa_sse41.a','src/compiler/glsl/libglsl.a',
    'src/compiler/glsl/glcpp/libglcpp.a','src/util/libmesa_util.a','src/util/libmesa_util_sse41.a',
    'src/util/blake3/libblake3.a','src/c11/impl/libmesa_util_c11.a','src/compiler/nir/libnir.a',
    'src/compiler/libcompiler.a','src/compiler/spirv/libvtn.a','src/gallium/auxiliary/libgallium.a',
    'src/gallium/winsys/sw/null/libws_null.a','src/mapi/shared-glapi/libglapi.a',
    'src/gallium/drivers/softpipe/libsoftpipe.a')]
library=out/'renderer.so'
egl=[]
if args.egl_core:
    core=args.egl_core.resolve()
    assert core.is_relative_to((root/'build/egl-core-evaluation').resolve())
    core_report=json.loads((core/'report.json').read_text())
    assert core_report['driver_compiled'] and not core_report['failed_units']
    assert core_report['renderer_probe']==str(probe)
    egl=[str(core/'libmyos-egl-core.a')]
# Only actual Mesa objects and native-tested pure SDK providers: NEVER the
# configure-only platform runtime/TLS archive. Runtime imports bind in MyOS.
subprocess.run(['ld','-shared','--gc-sections','--hash-style=both','-Bsymbolic',
                '-z','now','-z','relro','-z','noexecstack','-e','_start',
                '-o',str(library),str(fixture),str(font_object),str(finalize),str(environment),str(process_exit),
                str(mesa/'src/gallium/targets/osmesa/libOSMesa.so.8.0.0.p/osmesa_target.c.o'),
                *trace,'--start-group',
                *egl,*map(str,archives),str(containers['library']),str(math['static_library']),
                *[str(item['library'] if 'library' in item else item['out']/'libmyos-sort.a')
                  for item in sdk],'--end-group'],check=True)
assert '(NEEDED)' not in subprocess.check_output(['readelf','-dW',str(library)],text=True)
(out/'undefined.txt').write_text(subprocess.check_output(['nm','-u',str(library)],text=True))
disk=out/'assets.img';create(disk,['renderer='+str(library)])
boot=root/'build'/('myos-uefi.img' if args.uefi else 'myos-current.img')
def digest(path):
    with path.open('rb') as stream:return hashlib.file_digest(stream,'sha256').hexdigest()
before=digest(boot);image=out/'frozen-boot.img';shutil.copyfile(boot,image)
options=dict(ram=args.ram,boot_image=image,asset_disk=disk)
if args.uefi:options.update(boot_bus='ide',firmware=(Path('/usr/share/OVMF/OVMF_CODE_4M.fd'),
                                                    Path('/usr/share/OVMF/OVMF_VARS_4M.fd')))
print('Native renderer fixture:',out,'bytes:',library.stat().st_size,'RAM MiB:',args.ram,flush=True)
vm=VM('native-renderer'+('-egl-window' if args.egl_window else '-egl' if args.egl_core else '-gles' if args.gles else '')+
      ('-uefi' if args.uefi else ''),**options)
try:
    vm.wait('myos> ')
    for launch in range(args.launches):
        offset=len(vm.log.read_text());type_text(vm,'elf live renderer\n')
        end=time.monotonic()+args.startup_timeout
        while time.monotonic()<end:
            output=vm.log.read_text()[offset:]
            assert 'PANIC' not in output and 'USER FAULT' not in output,output
            assert not output.endswith('myos> '),output
            if 'Native OSMesa frame ready\n' in output:break
            time.sleep(.05)
        else:raise AssertionError(vm.log.read_text()[-6000:])
        color_checks=[
            (692,524,(255,0,0)),(712,524,(0,255,0)),
            (692,544,(0,0,255)),(712,544,(255,255,0))]
        if args.egl_window:
            # GLES coordinates are bottom-left; native framebuffer is top-left.
            color_checks=[(x,1071-y,rgb) for x,y,rgb in color_checks]
        frame_shot(vm,f'{vm.name}-{launch}-present.ppm',color_checks)
        glyphs=font.read_bytes()
        checks=[(688+character*8+column,552+row,(value,value,value))
                for character,code in enumerate(b'MyOS') for row in range(16)
                for column in range(8)
                for value in [128 if glyphs[code*16+row]&(128>>column) else 0]]
        frame_shot(vm,f'{vm.name}-{launch}-text.ppm',checks)
        vm.key('a');end=time.monotonic()+30
        while time.monotonic()<end:
            output=vm.log.read_text()[offset:]
            if output.endswith('myos> '):break
            time.sleep(.05)
        else:raise AssertionError(vm.log.read_text()[-6000:])
        assert 'ELF exited with status 90' in output and 'teardown PASS' in output,output
        if args.gles:assert 'Native GLES profile/desktop GLSL rejection checked' in output,output
        if args.egl_core:assert 'Native EGL core/error/current/deferred destroy checked' in output,output
        if args.egl_core:assert 'Native EGL last-core-pin cleanup checked' in output,output
        if args.egl_core:assert 'Native EGL requested-version/debug callback/surfaceless FBO checked' in output,output
        if args.egl_core:assert 'Native EGL real fence/eight waiters/query/errors/final-pin checked' in output,output
        if args.egl_core:assert 'Native EGL shared2D/cube6/3Dlayer/renderbuffer/orphan/final-pin checked' in output,output
        if args.egl_window:assert 'Native EGL window swap checked' in output,output
        assert 'Native glyph atlas/alpha blend/all512 text pixels checked' in output,output
        assert_live_resources(output)
    print('PASS: actual native Mesa context/GLSL/VBO/texture pixels/TLS/present/input/teardown'
          ' and exact resources/relaunch; profile='+('GLES' if args.gles else 'desktop GL')+
          '; '+('EGL1.4 native window swap; ' if args.egl_window else 'EGL1.4 pbuffer/context only; ' if args.egl_core else 'NOT EGL; ')+
          'NOT Cobalt.',out,flush=True)
except BaseException:
    # Capture the real running CPU before teardown; never infer a deadlock from
    # a silent log alone. Map user RIPs back to the exact linked target image.
    with (out/'failure-registers.log').open('w') as diagnostic:
        for _ in range(16):
            registers=vm.command('human-monitor-command',{'command-line':'info registers'})
            diagnostic.write(registers+'\n');diagnostic.flush()
            rip=re.search(r'RIP=([0-9a-fA-F]+)',registers)
            if rip and 0x8000000000<=int(rip[1],16)<0x8008000000:
                location=subprocess.check_output(['addr2line','-f','-e',str(library),
                    hex(int(rip[1],16)-0x8000000000)],text=True)
                diagnostic.write(location+'\n');diagnostic.flush()
            time.sleep(.05)
    print('Native renderer failure diagnostics:',out/'failure-registers.log',flush=True)
    raise
finally:vm.close()
assert digest(boot)==before==digest(image)
