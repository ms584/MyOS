"""Compile standard-media objects with MyOS headers, never host libraries."""
from pathlib import Path
import subprocess,sys
root=Path(__file__).resolve().parents[1]
args=sys.argv[1:]
if any(x in args for x in ('--version','-dumpmachine','-dumpversion')):
    raise SystemExit(subprocess.run(['gcc',*args]).returncode)
if not any(x in args for x in ('-c','-E','-S','-M','-MM')):
    raise SystemExit('media compiler rejects linking; use audited ld')
for x in args:
    if x.startswith(('-l','-L','@')) or x.endswith(('.a','.so')):
        raise SystemExit('media compiler rejects host library: '+x)
intrinsics=subprocess.check_output(['gcc','-print-file-name=include'],text=True).strip()
flags=['-nostdinc','-isystem',intrinsics,'-I'+str(root/'user/sdk'),
       '-I'+str(root/'build/media-evaluation/math-include'),
       '-ffreestanding','-fno-builtin','-fPIC','-mno-red-zone',
       '-fno-stack-protector','-fno-asynchronous-unwind-tables','-fno-unwind-tables',
       '-D__myos__=1','-U__linux__','-U__linux','-Ulinux','-U__gnu_linux__']
raise SystemExit(subprocess.run(['gcc',*flags,*args]).returncode)
