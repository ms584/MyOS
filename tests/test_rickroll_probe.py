"""Optional native MyOS transport probe for the Rickroll watch URL.

This is deliberately not a YouTube playback acceptance test.
"""
import re
import sys
sys.path.insert(0, "tests")
from test_system import VM
from test_network import send

vm = VM("phase32-rickroll-gate", ram=128, nic=True, cpu="max")
try:
    vm.wait("myos> ", 30)
    send(vm, "net up")
    vm.wait("IP 10.0.2.15", 15)
    for letter in "net https-count www.youtube.com 443 /watch?v=dQw4w9WgXcQ":
        if letter == "?":
            vm.key("shift", "slash")
        elif letter == "=":
            vm.key("equal")
        elif letter.isupper():
            vm.key("shift", letter.lower())
        else:
            vm.key({" ": "spc", ".": "dot", "/": "slash", "-": "minus"}.get(letter, letter))
    vm.key("ret")
    log = vm.wait("HTTPS verified; bytes=", 150)
    assert "PANIC" not in log
    assert "myos> net https-count www.youtube.com 443 /watch?v=dQw4w9WgXcQ" in log
    match = re.search(r"HTTPS verified; bytes=(\d+); FNV64=", log)
    assert match and int(match.group(1)) > 65536, log
    print("PASS: MyOS received Rickroll watch-page HTTPS response; video playback unsupported", flush=True)
finally:
    vm.close()
