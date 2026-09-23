"""Compatibility entry point for the x86-64 test suite."""
import subprocess
import sys
subprocess.run([sys.executable, "tests/test_system.py", "--suite", "exceptions"], check=True)
