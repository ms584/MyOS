"""Run a command while appending its actual output to build/activity.log.

Usage: python3 tools/run_logged.py -- python3 tests/test_system.py
View live in a second terminal: tail -f build/activity.log
"""
from datetime import datetime
import os
from pathlib import Path
import shlex
import subprocess
import sys

command=sys.argv[1:]
if command[:1]==['--']:command=command[1:]
if not command:raise SystemExit('Provide a command after --')
Path('build').mkdir(exist_ok=True)
env=dict(os.environ,MYOS_TRACE='1',PYTHONUNBUFFERED='1')
with Path('build/activity.log').open('a',buffering=1) as log:
    def emit(text):
        log.write(text);log.flush()
        sys.stdout.write(text);sys.stdout.flush()
    emit(f'\n[{datetime.now().astimezone().isoformat()}] cwd={Path.cwd()}\n$ {shlex.join(command)}\n')
    try:
        process=subprocess.Popen(command,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,
                                 text=True,bufsize=1,env=env)
        for line in process.stdout:emit(line)
        code=process.wait()
    except BaseException as error:
        emit(f'RUNNER ERROR: {error}\n')
        raise
    emit(f'[{datetime.now().astimezone().isoformat()}] exit={code}\n')
raise SystemExit(code)
