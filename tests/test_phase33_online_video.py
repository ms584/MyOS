"""Native HTTPS-to-framebuffer/SB16 MYV1 smoke test; not YouTube acceptance."""
import array
import socket
import ssl
import subprocess
import sys
import threading
import wave
from pathlib import Path

sys.path.insert(0, "tests")
from test_system import VM, OUT, build
from test_network import send, wait_count
from test_video import shot

CERTS = Path("security/certs")
CAPTURE = OUT / "phase33-online-capture.wav"
DISK = OUT / "phase33-online-demo.img"

def server(body, cert="test-server.pem", declared=None):
    listener = socket.socket()
    listener.bind(("127.0.0.1", 0))
    listener.listen(1)
    listener.settimeout(35)
    port = listener.getsockname()[1]
    errors = []

    def worker():
        try:
            context = ssl.SSLContext(ssl.PROTOCOL_TLS_SERVER)
            context.minimum_version = context.maximum_version = ssl.TLSVersion.TLSv1_2
            context.set_ciphers("ECDHE-ECDSA-AES128-GCM-SHA256")
            context.load_cert_chain(CERTS / cert, CERTS / "test-server.key")
            raw, _ = listener.accept()
            raw.settimeout(15)
            with context.wrap_socket(raw, server_side=True) as conn:
                assert b"GET /clip.myv HTTP/1.0" in conn.recv(1024)
                response = (b"HTTP/1.0 200 OK\r\nContent-Type: application/octet-stream\r\n"
                            + b"Content-Length: " + str(len(body) if declared is None else declared).encode()
                            + b"\r\n\r\n" + body)
                conn.sendall(response)
        except (ssl.SSLError, OSError, AssertionError) as error:
            errors.append(error)
        finally:
            listener.close()

    thread = threading.Thread(target=worker, daemon=True)
    thread.start()
    return port, thread, errors

def main():
    DISK.unlink(missing_ok=True)
    subprocess.run(["python3", "tools/make_video_disk.py", str(DISK)], check=True)
    body = Path("build/phase31-demo.myv").read_bytes()
    CAPTURE.unlink(missing_ok=True)
    try:
        build(28)
        vm = VM("phase33-online-video", ram=64, nic=True, cpu="max", audio=CAPTURE)
        try:
            vm.wait("myos> ", 30)
            send(vm, "net up")
            vm.wait("IP 10.0.2.15", 15)

            port, thread, errors = server(body)
            send(vm, f"video net 10.0.2.2 {port} /clip.myv")
            vm.wait("VIDEO FRAME 0 tick", 25)
            first = shot(vm, "phase33-online-first.ppm")
            assert first(280, 260) == (0xf5, 0xb6, 0x42)
            vm.wait("VIDEO FRAME 15 tick", 8)
            moved = shot(vm, "phase33-online-moved.ppm")
            assert moved(280, 260) == (0x10, 0x20, 0x38)
            vm.wait("VIDEO COMPLETE", 8)
            vm.wait("myos> ", 5)
            thread.join(5)
            assert not errors, errors

            control_port, control_thread, control_errors = server(body)
            send(vm, f"video net 10.0.2.2 {control_port} /clip.myv")
            wait_count(vm, "VIDEO FRAME 0 tick", 2, 25)
            vm.key("spc")
            vm.wait("VIDEO PAUSED", 5)
            vm.key("right")
            vm.wait("VIDEO SEEK frame ", 5)
            seeked = shot(vm, "phase33-online-seek-right.ppm")
            assert seeked(280, 260) == (0x10, 0x20, 0x38)
            vm.key("left")
            wait_count(vm, "VIDEO SEEK frame ", 2, 5)
            rewound = shot(vm, "phase33-online-seek-left.ppm")
            assert rewound(280, 260) == (0xf5, 0xb6, 0x42)
            vm.key("spc")
            vm.wait("VIDEO RESUMED", 5)
            vm.key("esc")
            vm.wait("VIDEO STOPPED", 5)
            vm.wait("myos> ", 5)
            control_thread.join(5)
            assert not control_errors, control_errors
            print("PASS: online MYV1 pause, seek both directions, resume and stop", flush=True)

            bad_port, bad_thread, bad_errors = server(body, declared=len(body)+1)
            send(vm, f"video net 10.0.2.2 {bad_port} /clip.myv")
            vm.wait("Video: invalid HTTPS clip response", 20)
            vm.wait("myos> ", 5)
            bad_thread.join(5)
            assert not bad_errors, bad_errors

            wrong_port, wrong_thread, _ = server(body, cert="wrong-host.pem")
            send(vm, f"video net 10.0.2.2 {wrong_port} /clip.myv")
            vm.wait("Video: verified HTTPS fetch failed", 20)
            vm.wait("myos> ", 5)
            wrong_thread.join(5)
            assert "PANIC" not in vm.log.read_text()
        finally:
            vm.close()
    finally:
        build()

    with wave.open(str(CAPTURE), "rb") as stream:
        data = stream.readframes(stream.getnframes())
    samples = array.array("h", data)
    assert max(abs(value) for value in samples) > 1000
    print("PASS: MyOS native HTTPS MYV1 rendered moving frames and SB16 sound; malformed length and wrong cert rejected", flush=True)

if __name__ == "__main__":
    main()
