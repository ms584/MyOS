"""Compile pinned Mesa C11 against MyOS-only headers and test in a frozen VM.

This is SDK groundwork, not native EGL/GLES or Cobalt acceptance.
"""
import argparse
import hashlib
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile

root = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(root / 'tests'))
sys.path.insert(0, str(root / 'tools'))
from test_system import VM
from test_elf_dynamic import long_command, assert_live_resources, cancel_live
from make_asset_disk import create


def digest(path):
    with path.open('rb') as stream:
        return hashlib.file_digest(stream, 'sha256').hexdigest()


def main(uefi=False):
    for name, expected in (
        ('threads.h', '052bd00a9be1cdf1e196cca65782f84e5aa91070f50b0d11f22d5a5c1111f8fc'),
        ('time.h', '2206f6ba6f339b9574f20c26ad21473f0094cbef4df6ef20c3824c47422039bb'),
        ('impl/threads_posix.c', '362fb5312e61aafb008075e66a9bff81ad74b2d1c6da3340cdcbd50bc92583b0')):
        assert digest(root / 'vendor/mesa-c11/src/c11' / name) == expected, name
    base = root / 'build/c11-evaluation'
    base.mkdir(parents=True, exist_ok=True)
    out = Path(tempfile.mkdtemp(prefix='native-', dir=base))
    boot = root / 'build' / ('myos-uefi.img' if uefi else 'myos-current.img')
    image = out / 'frozen-boot.img'
    before = digest(boot)
    shutil.copyfile(boot, image)
    assert before == digest(boot) == digest(image), 'Shared image changed during snapshot'
    print('Immutable native boot image:', before, flush=True)
    source = out / 'threads_posix.c'
    shutil.copyfile(root / 'vendor/mesa-c11/src/c11/impl/threads_posix.c', source)
    relative = str(out.relative_to(root))
    for check in (True, False):
        subprocess.run(['git', 'apply', *(['--check'] if check else []),
                        '--directory=' + relative, 'tools/mesa_c11_target.patch'],
                       cwd=root, check=True)
    compiler_include = subprocess.check_output(['gcc', '-print-file-name=include'], text=True).strip()
    flags = ['-m64', '-std=c11', '-Os', '-fPIC', '-ffreestanding', '-fno-builtin',
             '-mno-red-zone', '-fno-stack-protector', '-fno-asynchronous-unwind-tables',
             '-fno-unwind-tables', '-Wall', '-Wextra', '-Werror', '-nostdinc',
             '-isystem', compiler_include, '-Iuser/sdk', '-Ivendor/mesa-c11/src',
             '-DHAVE_PTHREAD', '-DHAVE_STRUCT_TIMESPEC']
    objects = []
    for name, src, extra in (
        ('threads', source, ['-Wno-unused-function']),
        ('time', root / 'user/platform-c11-time.c', [])):
        obj = out / (name + '.o')
        subprocess.run(['gcc', *flags, *extra, '-c', str(src), '-o', str(obj)], cwd=root, check=True)
        objects.append(str(obj))
    specs = []
    for name, defines in (('c11', []), ('cancel', ['-DC11_CANCEL=1'])):
        fixture = out / (name + '.o')
        subprocess.run(['gcc', *flags, *defines, '-c', str(root / 'user/elf-c11.c'),
                        '-o', str(fixture)], cwd=root, check=True)
        library = out / (name + '.so')
        subprocess.run(['ld', '-shared', '--hash-style=both', '-z', 'now', '-z', 'relro',
                        '-e', '_start', '-o', str(library), str(fixture), *objects], check=True)
        assert '(NEEDED)' not in subprocess.check_output(['readelf', '-dW', str(library)], text=True)
        specs.append(f'{name}={library}')
    disk = out / 'assets.img'
    create(disk, specs)
    options = dict(ram=64, boot_image=image, asset_disk=disk)
    if uefi:
        options.update(ram=128, boot_bus='ide', firmware=(
            Path('/usr/share/OVMF/OVMF_CODE_4M.fd'), Path('/usr/share/OVMF/OVMF_VARS_4M.fd')))
    vm = VM('c11-sdk-uefi' if uefi else 'c11-sdk', **options)
    try:
        vm.wait('myos> ')
        for _ in range(3):
            result = long_command(vm, 'elf live c11')
            assert 'ELF exited with status 70' in result, result
            assert 'native Mesa C11 pthread ABI/exit/once/TSS/timed waits PASS' in result, result
            assert_live_resources(result)
        cancel_live(vm, 'cancel', 'native C11 blocked cancel ready', timeout=15)
        result = long_command(vm, 'elf live c11')
        assert 'ELF exited with status 70' in result, result
        assert_live_resources(result)
        print('PASS: native pinned Mesa C11 ABI, eight workers/failed ninth allocation, '
              'once/TSS/explicit signed exit, clocks/sleep/errors, timed mutex/conditions, '
              'blocked-worker cancellation/relaunch and exact page/heap resources. '
              'NOT renderer/Cobalt acceptance.', flush=True)
    finally:
        vm.close()
    assert digest(image) == before == digest(boot), 'Boot image changed'


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--uefi', action='store_true')
    main(parser.parse_args().uefi)
