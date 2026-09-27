"""Optional live YouTube HTTPS streaming probe in native MyOS."""
import re
import sys
sys.path.insert(0, "tests")
from test_system import VM
from test_network import send

def main():
    vm = VM("phase32-youtube-stream-live", ram=128, nic=True, cpu="max")
    try:
        vm.wait("myos> ", 30)
        send(vm, "net up")
        vm.wait("IP 10.0.2.15", 15)
        send(vm, "net https-count www.youtube.com 443 /")
        log = vm.wait("HTTPS verified; bytes=", 150)
        match = re.search(r"HTTPS verified; bytes=(\d+); FNV64=", log)
        assert match and int(match.group(1)) > 65536, log
        assert "PANIC" not in log
        print("PASS: MyOS natively streamed verified YouTube HTTPS beyond 64 KiB; playback unsupported", flush=True)
    finally:
        vm.close()

if __name__ == "__main__":
    main()
