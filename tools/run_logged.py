"""Run a command with bounded live logging.

Usage: python3 tools/run_logged.py -- python3 tests/test_system.py
View from a separate, unwrapped terminal: tail -F build/activity.log
The current log and two older logs are limited to 16 MiB each.
Never pass activity.log to this wrapper: that can feed output back into itself.
"""
import codecs
from datetime import datetime
import fcntl
import os
from pathlib import Path
import shlex
import subprocess
import sys

LOG_LIMIT=16*1024*1024
READ_SIZE=64*1024

def append_log(directory,data,limit=LOG_LIMIT):
    """Lock rotation and append together so concurrent runners share one budget."""
    target=directory/'activity.log'
    with (directory/'activity.lock').open('ab') as lock:
        fcntl.flock(lock,fcntl.LOCK_EX)
        if target.exists() and target.stat().st_size+len(data)>limit:
            oldest=directory/'activity.log.2'
            oldest.unlink(missing_ok=True)
            previous=directory/'activity.log.1'
            if previous.exists():previous.replace(oldest)
            target.replace(previous)
        with target.open('ab') as log:log.write(data)

def main(arguments=None):
    command=list(sys.argv[1:] if arguments is None else arguments)
    if command[:1]==['--']:command=command[1:]
    if not command:raise SystemExit('Provide a command after --')
    if any('activity.log' in argument for argument in command):
        raise SystemExit('Refusing activity.log feedback; inspect logs outside run_logged.py')
    directory=Path('build');directory.mkdir(exist_ok=True)
    env=dict(os.environ,MYOS_TRACE='1',PYTHONUNBUFFERED='1')
    def emit(text):
        data=text.encode('utf-8',errors='replace')
        for offset in range(0,len(data),READ_SIZE):append_log(directory,data[offset:offset+READ_SIZE])
        sys.stdout.write(text);sys.stdout.flush()
    emit(f'\n[{datetime.now().astimezone().isoformat()}] cwd={Path.cwd()}\n$ {shlex.join(command)}\n')
    try:
        process=subprocess.Popen(command,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,env=env)
        decoder=codecs.getincrementaldecoder('utf-8')(errors='replace')
        while True:
            chunk=os.read(process.stdout.fileno(),READ_SIZE)
            if not chunk:break
            emit(decoder.decode(chunk))
        emit(decoder.decode(b'',final=True))
        code=process.wait()
    except BaseException as error:
        emit(f'RUNNER ERROR: {error}\n')
        if 'process' in locals():
            process.terminate()
            try:process.wait(timeout=5)
            except subprocess.TimeoutExpired:process.kill();process.wait(timeout=5)
        raise
    emit(f'[{datetime.now().astimezone().isoformat()}] exit={code}\n')
    return code

if __name__=='__main__':raise SystemExit(main())
