"""Bounded logging and the recursive-log failure regression."""
import importlib.util
from pathlib import Path
import subprocess
import sys
import tempfile

ROOT=Path(__file__).resolve().parents[1]
spec=importlib.util.spec_from_file_location('run_logged',ROOT/'tools/run_logged.py')
logger=importlib.util.module_from_spec(spec);spec.loader.exec_module(logger)

def main():
    with tempfile.TemporaryDirectory(prefix='myos-log-test-') as temp:
        directory=Path(temp)
        for index in range(12):logger.append_log(directory,bytes([index])*32,limit=64)
        files=sorted(directory.glob('activity.log*'))
        assert len(files)==3
        assert all(path.stat().st_size<=64 for path in files)
        assert (directory/'activity.log').read_bytes()==bytes([10])*32+bytes([11])*32
        denied=subprocess.run([sys.executable,str(ROOT/'tools/run_logged.py'),'--','cat','build/activity.log'],
                              cwd=directory,capture_output=True,text=True)
        assert denied.returncode and 'feedback' in denied.stderr
        assert not (directory/'build/activity.log').exists()
        result=subprocess.run([sys.executable,str(ROOT/'tools/run_logged.py'),'--',
                               sys.executable,'-c',"import sys; sys.stdout.write('x'*200000); sys.exit(7)"],
                              cwd=directory,capture_output=True,text=True)
        assert result.returncode==7 and result.stdout.count('x')>=200000
        assert 'exit=7' in (directory/'build/activity.log').read_text()
    print('PASS: bounded log rotation, no recursive feedback, long lines and exit status')

if __name__=='__main__':main()
