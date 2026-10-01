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
options, args = parser.parse_known_args()
if args[:1] == ['--']:
    args = args[1:]
# Meson version/target queries must remain the actual compiler's answers.
query = any(arg in ('--version', '-dumpmachine', '-dumpversion', '-print-search-dirs')
            or arg.startswith('-print-file-name=') for arg in args)
compiler = 'g++' if options.language == 'cpp' else 'gcc'
flags = [] if query else [
    '-nostdinc', '-isystem', str(options.intrinsics), '-I' + str(options.sdk),
    '-ffreestanding', '-fno-builtin', '-mno-red-zone', '-fno-stack-protector',
    '-fmax-errors=5',
    '-fno-asynchronous-unwind-tables', '-fno-unwind-tables', '-D__myos__=1',
    '-U__linux__', '-U__linux', '-Ulinux', '-U__gnu_linux__',
]
if not query and options.language == 'cpp':
    flags += ['-nostdinc++', '-fno-exceptions', '-fno-rtti']
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
tail = ['-nostdlib', '-Wl,-e,main', '-x', 'none', str(options.provider)] if link else []
sys.exit(subprocess.run([compiler, *flags, *args, *tail]).returncode)
