"""Pinned signal object layouts and native masks, NOT signal delivery."""
from pathlib import Path
import argparse
import hashlib
import shutil
import subprocess
import sys
import tempfile

root=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(root/'tests'));sys.path.insert(0,str(root/'tools'))
from test_system import VM
from test_elf_dynamic import long_command,assert_live_resources
from make_asset_disk import create

def digest(path):
    with path.open('rb') as stream:
        return hashlib.file_digest(stream,'sha256').hexdigest()

parser=argparse.ArgumentParser()
parser.add_argument('--uefi',action='store_true')
args=parser.parse_args()
base=root/'build/signal-evaluation';base.mkdir(parents=True,exist_ok=True)
out=Path(tempfile.mkdtemp(prefix='sdk-',dir=base))
header=root/'build/engine-evaluation/contracts/starboard_layer_posix_signal_abi_wrappers.h'
assert digest(header)=='b076da2463a4e03d8917323314e3383919875d777702477e675ba98bc1140e3e'
# Compare against the exact pinned definitions, not the host libc. The wrapper
# includes clock_t only for layout; on the pinned x64 ABI that field is long.
definitions=header.read_text().split('// --- MUSL-compatible definitions for signal handling ---')[1].split('// --- ABI Wrapper Functions ---')[0]
layout='''#include <signal.h>
#include <stddef.h>
typedef long clock_t;
'''+definitions+'''
#ifdef __cplusplus
#define VERIFY static_assert
#define ALIGNOF alignof
#else
#define VERIFY _Static_assert
#define ALIGNOF _Alignof
#endif
#define SAME(a,b) VERIFY((a)==(b), #a " != " #b)
SAME(sizeof(sigset_t),sizeof(musl_sigset_t));
SAME(ALIGNOF(sigset_t),ALIGNOF(musl_sigset_t));
SAME(sizeof(siginfo_t),sizeof(musl_siginfo_t));
SAME(ALIGNOF(siginfo_t),ALIGNOF(musl_siginfo_t));
SAME(sizeof(struct sigaction),sizeof(musl_sigaction));
SAME(ALIGNOF(struct sigaction),ALIGNOF(musl_sigaction));
SAME(offsetof(struct sigaction,sa_handler),offsetof(musl_sigaction,musl_sa_handler));
SAME(offsetof(struct sigaction,sa_sigaction),offsetof(musl_sigaction,musl_sa_sigaction));
SAME(offsetof(struct sigaction,sa_mask),offsetof(musl_sigaction,sa_mask));
SAME(offsetof(struct sigaction,sa_flags),offsetof(musl_sigaction,sa_flags));
SAME(offsetof(struct sigaction,sa_restorer),offsetof(musl_sigaction,sa_restorer));
SAME(offsetof(siginfo_t,si_code),offsetof(musl_siginfo_t,musl_si_code));
SAME(offsetof(siginfo_t,fields.common.first.identity.pid),offsetof(musl_siginfo_t,__si_fields.__si_common.__first.__piduid.musl_si_pid));
SAME(offsetof(siginfo_t,fields.common.second.value),offsetof(musl_siginfo_t,__si_fields.__si_common.__second.musl_si_value));
SAME(offsetof(siginfo_t,fields.common.second.child.utime),offsetof(musl_siginfo_t,__si_fields.__si_common.__second.__sigchld.musl_si_utime));
SAME(offsetof(siginfo_t,fields.fault.address),offsetof(musl_siginfo_t,__si_fields.__sigfault.musl_si_addr));
SAME(offsetof(siginfo_t,fields.fault.first.bounds.lower),offsetof(musl_siginfo_t,__si_fields.__sigfault.__first.__addr_bnd.musl_si_lower));
SAME(offsetof(siginfo_t,fields.fault.first.pkey),offsetof(musl_siginfo_t,__si_fields.__sigfault.__first.musl_si_pkey));
SAME(offsetof(siginfo_t,fields.poll.fd),offsetof(musl_siginfo_t,__si_fields.__sigpoll.musl_si_fd));
SAME(offsetof(siginfo_t,fields.system.arch),offsetof(musl_siginfo_t,__si_fields.__sigsys.musl_si_arch));
SAME(SA_NOCLDSTOP,MUSL_SA_NOCLDSTOP);SAME(SA_NOCLDWAIT,MUSL_SA_NOCLDWAIT);
SAME(SA_SIGINFO,MUSL_SA_SIGINFO);SAME(SA_ONSTACK,MUSL_SA_ONSTACK);
SAME(SA_RESTART,MUSL_SA_RESTART);SAME(SA_NODEFER,MUSL_SA_NODEFER);
SAME(SA_RESETHAND,MUSL_SA_RESETHAND);
'''
include=subprocess.check_output(['gcc','-print-file-name=include'],text=True).strip()
flags=['-m64','-Os','-fPIC','-ffreestanding','-fno-builtin','-mno-red-zone',
       '-fno-stack-protector','-fno-asynchronous-unwind-tables','-fno-unwind-tables',
       '-nostdinc','-isystem',include,'-I'+str(root/'user/sdk'),'-D__myos__=1',
       '-U__linux__','-U__linux','-Ulinux','-U__gnu_linux__',
       '-Wall','-Wextra','-Werror']
assets=[]
for compiler,language in [('gcc','c'),('g++','c++')]:
    extra=['-std=c11'] if language=='c' else ['-std=c++17','-nostdinc++','-fno-exceptions','-fno-rtti']
    comparison=out/(compiler+'-layout.'+('c' if language=='c' else 'cpp'))
    comparison.write_text(layout)
    subprocess.run([compiler,*flags,*extra,'-fsyntax-only',str(comparison)],check=True)
    obj=out/(compiler+'.o');library=out/(compiler+'.so')
    subprocess.run([compiler,*flags,*extra,'-x',language,'-c',str(root/'user/elf-signal-sdk.c'),'-o',str(obj)],check=True)
    subprocess.run(['ld','-shared','--hash-style=both','-z','now','-z','relro',
                    '-z','noexecstack','-e','_start','-o',str(library),str(obj)],check=True)
    assert '(NEEDED)' not in subprocess.check_output(['readelf','-dW',str(library)],text=True)
    symbols=subprocess.check_output(['nm','-D',str(library)],text=True)
    assert not any(line.split()[-1] in ('signal','sigaction','raise','pause','abort') for line in symbols.splitlines() if line.split())
    assets.append(f'{"c" if compiler=="gcc" else "cpp"}={library}')
disk=out/'assets.img';create(disk,assets)
boot=root/'build'/('myos-uefi.img' if args.uefi else 'myos-current.img')
image=out/'frozen-boot.img';before=digest(boot);shutil.copyfile(boot,image)
options=dict(ram=64,boot_image=image,asset_disk=disk)
if args.uefi:
    options.update(ram=128,boot_bus='ide',firmware=(
        Path('/usr/share/OVMF/OVMF_CODE_4M.fd'),Path('/usr/share/OVMF/OVMF_VARS_4M.fd')))
vm=VM('signal-sdk-uefi' if args.uefi else 'signal-sdk',**options)
try:
    vm.wait('myos> ')
    for _ in range(3):
        for name in ('c','cpp'):
            result=long_command(vm,'elf live '+name)
            assert 'ELF exited with status 89' in result,result
            assert 'native signal SDK masks/inheritance/isolation PASS (no delivery)' in result,result
            assert_live_resources(result)
    print('PASS: exact pinned signal layouts C11/C++17, all127 mask positions, '
          'bounds/errno/canaries, aliased pthread mask output/padding/unblockable '
          'signals,8-worker inheritance/preempted isolation/6 launches/exact '
          'resources. NOT signal delivery or new Cobalt bindings.',out,flush=True)
finally:
    vm.close()
assert before==digest(boot)==digest(image)
