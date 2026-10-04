"""Compiler boundary for Mesa's MyOS cross-configuration probe.

This is not a host-library compatibility shim. Link checks use only the real
MyOS runtime archive supplied by the caller; Linux libraries are rejected.
"""
import argparse
from pathlib import Path
import subprocess
import sys

parser = argparse.ArgumentParser()
parser.add_argument('--language', choices=('c', 'cpp'), required=True)
parser.add_argument('--sdk', type=Path, required=True)
parser.add_argument('--intrinsics', type=Path, required=True)
parser.add_argument('--provider', type=Path, required=True)
parser.add_argument('--math-include', type=Path)
parser.add_argument('--cxx-library', action='store_true',
                    help='Upstream C++ headers before SDK shims; hosted language profile, not host libraries')
options, args = parser.parse_known_args()
if options.cxx_library and options.language != 'cpp':
    parser.error('--cxx-library requires --language cpp')
if args[:1] == ['--']:
    args = args[1:]
# Meson version/target queries must remain the actual compiler's answers.
query = any(arg in ('--version', '-dumpmachine', '-dumpversion', '-print-search-dirs')
            or arg.startswith('-print-file-name=') for arg in args)
compiler = 'g++' if options.language == 'cpp' else 'gcc'
flags = [] if query else [
    '-nostdinc', '-isystem', str(options.intrinsics),
    *(['-idirafter', str(options.sdk)] if options.cxx_library else ['-I' + str(options.sdk)]),
    *([] if options.cxx_library else ['-ffreestanding']),
    '-fno-builtin', '-mno-red-zone', '-fno-stack-protector',
    '-fmax-errors=5',
    '-fno-asynchronous-unwind-tables', '-fno-unwind-tables', '-D__myos__=1',
    '-D_GNU_SOURCE=1',  # Mesa requires the sealed math header's extension constants.
    '-U__linux__', '-U__linux', '-Ulinux', '-U__gnu_linux__',
]
if options.math_include and not query:
    assert options.math_include.resolve().is_relative_to(options.provider.parent.resolve())
    # GCC <cmath> uses include_next: target C math must follow upstream C++
    # headers rather than precede them. Default C/renderer profile is unchanged.
    flags += (['-idirafter', str(options.math_include)] if options.cxx_library
              else ['-I' + str(options.math_include)])
if not query and options.language == 'cpp':
    flags += ['-nostdinc++', '-fno-exceptions', '-fno-rtti',
              '-D__STDC_LIMIT_MACROS=1', '-D__STDC_CONSTANT_MACROS=1']
for arg in args:
    tokens = arg[4:].split(',') if arg.startswith('-Wl,') else [arg]
    if any(token.startswith(('-l', '-L', '@')) or token in ('-Xlinker', '-fopenmp') or (
            token.endswith(('.a', '.so'))
            and not Path(token).resolve().is_relative_to(options.provider.parent.resolve()))
           for token in tokens):
        print('MyOS target probe rejects unverified library/search/response argument: ' + arg,
              file=sys.stderr)
        sys.exit(1)
link = not query and not any(arg in ('-c', '-E', '-S', '-fsyntax-only') for arg in args)
tail = (['-nostdlib', *([] if '-shared' in args else ['-Wl,-e,main']),
         '-x', 'none', str(options.provider)] if link else [])
sys.exit(subprocess.run([compiler, *flags, *args, *tail]).returncode)
