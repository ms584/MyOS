"""Packet-level unit evidence, supplementary to native QEMU transport tests."""
import subprocess
from pathlib import Path

def main():
    output=Path('build/system-tests/tcp-state');output.mkdir(parents=True,exist_ok=True)
    binary=output/'tcp-unit'
    subprocess.run(['gcc','-std=c11','-O1','-g','-fno-builtin','-Wall','-Wextra','-Werror','-Wno-misleading-indentation',
                    '-fsanitize=address,undefined','-fno-omit-frame-pointer','-Iinclude',
                    'net/tcp.c','tests/tcp-unit.c','-o',str(binary)],check=True)
    subprocess.run([str(binary)],check=True)
    print('PASS: TCP tuple/generation isolation, partial ACK/retransmit, sequence wrap, duplicate/out-of-order, bounded/zero window, FIN/RST, timeout and pool teardown (host state-machine test)')

if __name__=='__main__':main()
