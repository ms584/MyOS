"""Explicit non-root KVM experiment; no fallback and no guest-source change."""
import os
import subprocess

def install():
    assert os.geteuid() != 0, 'Never run this experiment as root'
    original = subprocess.Popen
    def launch(args, *positional, **kwargs):
        if isinstance(args, list) and args and args[0] == 'qemu-system-x86_64':
            args = list(args)
            assert '-accel' not in args and '-enable-kvm' not in args
            if '-cpu' in args:
                index = args.index('-cpu')
                assert args[index + 1] == 'max'
                args[index + 1] = 'host'
            else:
                args.extend(['-cpu', 'host'])
            args.extend(['-accel', 'kvm'])
            print('EXPLICIT_KVM_COMMAND', args, flush=True)
        return original(args, *positional, **kwargs)
    subprocess.Popen = launch
