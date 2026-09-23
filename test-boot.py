"""Boot the Phase 2.2 image and inspect physical VGA memory through QMP."""
import json
import pathlib
import socket
import subprocess
import time

root = pathlib.Path.cwd()
sockpath = root / 'build/phase22-qmp.sock'
dump = root / 'build/phase22-vga.bin'
if sockpath.exists():
    sockpath.unlink()
process = subprocess.Popen([
    'qemu-system-x86_64', '-display', 'none', '-monitor', 'none',
    '-serial', 'none', '-no-reboot',
    '-drive', 'format=raw,file=build/myos-phase22.img,if=floppy',
    '-boot', 'order=a', '-qmp', f'unix:{sockpath},server=on,wait=off'])
try:
    connection = socket.socket(socket.AF_UNIX)
    for _ in range(100):
        try:
            connection.connect(str(sockpath))
            break
        except (FileNotFoundError, ConnectionRefusedError):
            time.sleep(0.05)
    connection.settimeout(5)
    stream = connection.makefile('rwb')
    json.loads(stream.readline())

    def command(name, arguments=None):
        request = {'execute': name}
        if arguments:
            request['arguments'] = arguments
        stream.write(json.dumps(request).encode() + b'\n')
        stream.flush()
        while True:
            response = json.loads(stream.readline())
            if 'error' in response:
                raise RuntimeError(response)
            if 'return' in response:
                return response['return']

    command('qmp_capabilities')
    expected = ['MyOS Kernel', '32-bit Protected Mode', 'VGA driver initialized']
    for _ in range(60):
        command('human-monitor-command', {'command-line': f'pmemsave 0xb8000 4000 "{dump}"'})
        memory = dump.read_bytes()
        lines = [memory[row*160:(row+1)*160:2].decode('ascii', errors='replace')
                 for row in range(25)]
        if [line.rstrip() for line in lines[:3]] == expected:
            break
        time.sleep(0.1)
    assert [line.rstrip() for line in lines[:3]] == expected, lines[:3]
    assert all(line == ' ' * 80 for line in lines[3:]), 'Screen not cleared'
    assert all(attribute == 15 for attribute in memory[1::2]), 'Wrong color'
    print('PASS: QEMU boot, all three startup lines, cleared remaining screen, VGA attributes')
finally:
    process.terminate()
    process.wait(timeout=5)
    if sockpath.exists():
        sockpath.unlink()
