"""Native pinned Mesa thread utilities, not native graphics/Cobalt acceptance."""
import argparse
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile

root = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(root / 'tests'))
sys.path.insert(0, str(root / 'tools'))
from test_c11_sdk import digest
from test_system import VM
from test_elf_dynamic import long_command, assert_live_resources, cancel_live
from make_asset_disk import create


def main(uefi=False):
    for name, expected in (
        ('threads.h', '052bd00a9be1cdf1e196cca65782f84e5aa91070f50b0d11f22d5a5c1111f8fc'),
        ('time.h', '2206f6ba6f339b9574f20c26ad21473f0094cbef4df6ef20c3824c47422039bb'),
        ('impl/threads_posix.c', '362fb5312e61aafb008075e66a9bff81ad74b2d1c6da3340cdcbd50bc92583b0')):
        assert digest(root / 'vendor/mesa-c11/src/c11' / name) == expected, name
    for name, expected in (
        ('u_thread.c', '9fbebc6f182851a67f493288dd0a6b5c331bf076d8e7c485a7082058e97235ae'),
        ('u_thread.h', 'ebd25a27827c27a6035617e29e29b7a4b40b04dcac4c5aaf704fd69343995265'),
        ('macros.h', 'cb7761079fd529cae22c7c5959573ddccdad7ebc335f362431557f7e0ef19e06'),
        ('detect_os.h', '143d669562560a4613e47beb0e7e06c4dc608e7db156600600a6dccd7e87e7b8')):
        assert digest(root / 'vendor/mesa-c11/src/util' / name) == expected, name
    base = root / 'build/mesa-util-evaluation'
    base.mkdir(parents=True, exist_ok=True)
    out = Path(tempfile.mkdtemp(prefix='native-', dir=base))
    boot = root / 'build' / ('myos-uefi.img' if uefi else 'myos-current.img')
    image = out / 'frozen-boot.img'
    before = digest(boot)
    shutil.copyfile(boot, image)
    assert before == digest(boot) == digest(image), 'Shared image changed during snapshot'
    print('Immutable native boot image:', before, flush=True)
    shutil.copytree(root / 'vendor/mesa-c11/src', out / 'src')
    shutil.copyfile(out / 'src/c11/impl/threads_posix.c', out / 'threads_posix.c')
    for patch in ('mesa_c11_target.patch', 'mesa_thread_util_target.patch'):
        for check in (True, False):
            subprocess.run(['git', 'apply', *(['--check'] if check else []),
                            '--directory=' + str(out.relative_to(root)), 'tools/' + patch],
                           cwd=root, check=True)
    include = subprocess.check_output(['gcc', '-print-file-name=include'], text=True).strip()
    flags = ['-m64', '-std=c11', '-Os', '-fPIC', '-ffreestanding', '-fno-builtin',
             '-mno-red-zone', '-fno-stack-protector', '-fno-asynchronous-unwind-tables',
             '-fno-unwind-tables', '-Wall', '-Wextra', '-Werror', '-nostdinc',
             '-isystem', include, '-Iuser/sdk', '-I' + str(out / 'src'),
             '-DHAVE_PTHREAD', '-DHAVE_STRUCT_TIMESPEC', '-D__myos__=1',
             '-U__linux__', '-U__linux', '-Ulinux', '-U__gnu_linux__']
    objects = []
    for name, source, extra in (
        ('threads', out / 'threads_posix.c', ['-Wno-unused-function']),
        ('util', out / 'src/util/u_thread.c', ['-Wno-unused-parameter']),
        ('time', root / 'user/platform-c11-time.c', []),
        ('string', root / 'user/platform-sdk-string.c', [])):
        obj = out / (name + '.o')
        subprocess.run(['gcc', *flags, *extra, '-c', str(source), '-o', str(obj)], cwd=root, check=True)
        objects.append(str(obj))
    specs = []
    for name, defines in (('util', []), ('cancel', ['-DUTIL_CANCEL=1']), ('fault', ['-DUTIL_FAULT=1'])):
        fixture = out / (name + '-fixture.o')
        subprocess.run(['gcc', *flags, *defines, '-c', str(root / 'user/elf-mesa-thread-util.c'),
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
    vm = VM('mesa-util-uefi' if uefi else 'mesa-util', **options)
    try:
        vm.wait('myos> ')
        for _ in range(3):
            result = long_command(vm, 'elf live util')
            assert 'ELF exited with status 71' in result, result
            assert 'native Mesa util barrier/semaphore/TLS/masks/names PASS' in result, result
            assert_live_resources(result)
        result = long_command(vm, 'elf live fault')
        assert 'USER FAULT' in result and 'vector=6 ' in result, result
        assert_live_resources(result)
        cancel_live(vm, 'cancel', 'native Mesa util barrier cancel ready', timeout=15)
        result = long_command(vm, 'elf live util')
        assert 'ELF exited with status 71' in result, result
        assert_live_resources(result)
        print('PASS: native Mesa C11 thread util, reusable barriers/single serial thread, '
              'semaphore, native dispatch TLS isolation, masks/names, invalid-count fault, '
              'cancel/relaunch/exact resources. NOT graphics/Cobalt acceptance.', flush=True)
    finally:
        vm.close()
    assert digest(image) == before == digest(boot), 'Boot image changed'


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--uefi', action='store_true')
    main(parser.parse_args().uefi)
