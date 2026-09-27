"""Native MyOS TLS streaming test in QEMU; no host playback."""
import re
import socket
import ssl
import sys
import threading
from pathlib import Path

sys.path.insert(0, "tests")
from test_system import VM, build
from test_network import send

CERTS = Path("security/certs")
BODY = bytes(range(256)) * 1024
RESPONSE = (b"HTTP/1.0 200 OK\r\nContent-Type: application/octet-stream\r\n"
            + b"Content-Length: " + str(len(BODY)).encode() + b"\r\n\r\n" + BODY)

def fnv64(data):
    value = 14695981039346656037
    for byte in data:
        value = ((value ^ byte) * 1099511628211) & ((1 << 64) - 1)
    return value

def server():
    listener = socket.socket()
    listener.bind(("127.0.0.1", 0))
    listener.listen(1)
    listener.settimeout(40)
    port = listener.getsockname()[1]
    errors = []
    def worker():
        try:
            context = ssl.SSLContext(ssl.PROTOCOL_TLS_SERVER)
            context.minimum_version = context.maximum_version = ssl.TLSVersion.TLSv1_2
            context.set_ciphers("ECDHE-ECDSA-AES128-GCM-SHA256")
            context.load_cert_chain(CERTS / "test-server.pem", CERTS / "test-server.key")
            raw, _ = listener.accept()
            raw.settimeout(60)
            with context.wrap_socket(raw, server_side=True) as conn:
                assert b"GET /large HTTP/1.0" in conn.recv(1024)
                conn.sendall(RESPONSE)
        except Exception as error:
            errors.append(error)
        finally:
            listener.close()
    thread = threading.Thread(target=worker, daemon=True)
    thread.start()
    return port, thread, errors

def main():
    try:
        build(28)
        vm = VM("phase32-native-stream", ram=64, nic=True, cpu="max")
        try:
            vm.wait("myos> ", 30)
            send(vm, "net up")
            vm.wait("IP 10.0.2.15", 15)
            port, thread, errors = server()
            send(vm, f"net https-count 10.0.2.2 {port} /large")
            log = vm.wait("HTTPS verified; bytes=", 120)
            match = re.search(r"HTTPS verified; bytes=(\d+); FNV64=(?:0x)?([0-9A-Fa-f]+)", log)
            assert match, log
            assert int(match.group(1)) == len(RESPONSE), match.groups()
            assert int(match.group(2), 16) == fnv64(RESPONSE), match.groups()
            thread.join(5)
            assert not errors, errors
            print("PASS: MyOS streamed and verified 256 KiB+ HTTPS response with no full-page buffer", flush=True)
        finally:
            vm.close()
    finally:
        build()

if __name__ == "__main__":
    main()
