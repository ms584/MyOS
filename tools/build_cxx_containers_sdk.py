"""Real GCC prime-rehash and no-exception target allocation, not full libstdc++."""
from pathlib import Path
import argparse
import shutil
import subprocess
import tempfile

root=Path(__file__).resolve().parents[1]

def build(configuration,math_provider):
    cfg=Path(configuration).resolve();provider=Path(math_provider).resolve()
    assert cfg.is_relative_to((root/'build/cxx-target-evaluation').resolve())
    assert provider.is_relative_to((root/'build/renderer-evaluation').resolve()) and provider.is_file()
    source=cfg/'gcc-15.2.0/libstdc++-v3'
    assert (source.parent/'gcc/BASE-VER').read_text().strip()=='15.2.0'
    header=cfg/'build/include/x86_64-unknown-myos/bits/c++config.h'
    assert '#define _GLIBCXX_HAS_GTHREADS 1' in header.read_text()
    base=root/'build/cxx-container-evaluation';base.mkdir(parents=True,exist_ok=True)
    out=Path(tempfile.mkdtemp(prefix='sdk-',dir=base))
    for name in ('COPYING3','COPYING.RUNTIME'):
        shutil.copyfile(source.parent/name,out/name)
    include=subprocess.check_output(['gcc','-print-file-name=include'],text=True).strip()
    flags=['-m64','-Os','-std=c++17','-fPIC','-fno-builtin','-mno-red-zone',
           '-fno-stack-protector','-fno-asynchronous-unwind-tables','-fno-unwind-tables',
           '-fno-exceptions','-fno-rtti','-nostdinc','-nostdinc++','-isystem',include,
           '-isystem',str(cfg/'build/include'),'-isystem',str(cfg/'build/include/x86_64-unknown-myos'),
           '-isystem',str(source/'libsupc++'),'-idirafter',str(root/'user/sdk'),
           '-idirafter',str(provider.parent/'math-include'),'-D__myos__=1','-D_GNU_SOURCE=1',
           '-U__linux__','-U__linux','-Ulinux','-U__gnu_linux__','-Wall','-Wextra','-Werror',
           '-ffunction-sections','-fdata-sections']
    primitives=out/'primitives.o'
    subprocess.run(['gcc','-m64','-std=c11','-Os','-fPIC','-ffreestanding','-fno-builtin',
                    '-mno-red-zone','-fno-stack-protector','-nostdinc','-isystem',include,
                    '-I'+str(root/'user/sdk'),'-c',str(root/'user/platform-sdk-string.c'),
                    '-o',str(primitives)],check=True)
    # Never embed the configure-only runtime's fixed TLS/data addresses in a
    # native ELF. Only these sealed pure math algorithms have no dependencies.
    subprocess.run(['ar','x',str(provider),'floor.o','ceil.o'],cwd=out,check=True)
    objects=[primitives]
    for name in ('floor','ceil'):
        obj=out/(name+'.o');assert obj.is_file()
        assert not subprocess.check_output(['nm','-u',str(obj)],text=True).strip()
        objects.append(obj)
    cxx_objects=[]
    for name,path in [('allocation',root/'user/platform-cxx-allocation.cpp'),
                      ('assert',root/'user/platform-cxx-assert.cpp'),
                      ('hash',source/'src/c++11/hashtable_c++0x.cc')]:
        obj=out/(name+'.o')
        subprocess.run(['g++',*flags,'-c',str(path),'-o',str(obj)],check=True)
        cxx_objects.append(obj)
    objects+=cxx_objects
    library=out/'libmyos-cxx-containers.a'
    subprocess.run(['ar','rcs',str(library),*map(str,objects)],check=True)
    return dict(out=out,flags=flags,library=library,cxx_objects=cxx_objects)

if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--configuration',type=Path,required=True)
    parser.add_argument('--math-provider',type=Path,required=True)
    args=parser.parse_args();result=build(args.configuration,args.math_provider)
    print('CXX CONTAINER SDK COMPILE ONLY:',result['out'],'NOT full C++/native proof.',flush=True)
