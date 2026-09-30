# Phase 33.1 independently owned TCP streams

Status: dependency implementation, not Phase 33.1/33.2 closure. Cobalt has not
executed and HTML/JavaScript/DOM/input/HTTPS endurance acceptance is absent.
Do not start Phase 33.3. The pinned socket wrapper constants/signatures in
build/engine-evaluation/contracts/starboard_layer_posix_socket_abi_wrappers.h
were inspected at source 3befd332486952f6603ae3baf2a8713878d6e200.

## Ownership and transport

net/tcp.c owns a machine-wide pool of 32 streams. Each has independent peer,
local port, send/receive sequence, receive window, transmit queue, retry state,
EOF/shutdown/error and process owner. Handles include slot + generation and are
validated against the caller's process. This is not multiple descriptors sharing
the old single TCP peer/buffer. Ports 56000..60999 are reserved for the new pool;
the legacy serialized native HTTP/TLS client remains on its compatibility path.

Each live stream allocates an 8192-byte receive ring and 4096-byte send ring.
IRQ work polls at most eight NIC frames per timer tick, then advances the bounded
pool. Packet validation/checksums remain at the net.c boundary. Legacy packet
construction and NIC polling are IRQ-serialized on the supported single CPU;
new streams build independent packets. All new transport syscalls are
nonblocking and validate/copy through supervisor mappings, not unchecked user
addresses. No kernel networking wait enables interrupts inside INT 0x80.

ARP has three bounded retries; SYN has three 150-tick attempts; unacknowledged
data/FIN has bounded retries. Incoming sequence intervals are checked, old
prefixes trimmed, out-of-order data ACKed without delivery, and receive writes
bounded by advertised space. ACKs cannot consume unsent bytes. Partial ACKs
retransmit only the unacknowledged interval. Zero-window probes request an
updated window without claiming unsent data was acknowledged. RST is accepted
only at the expected receive sequence (or valid SYN acknowledgement).

The initial sender is deliberately conservative: one in-flight segment per
connection, bounded by peer MSS/window. It does not implement general congestion
control, SACK, TCP timestamps, IP fragment reassembly or a measured high-bandwidth
browser transport. Native engine performance still requires measurement.

## Shared descriptor and wait behavior

The runtime's existing descriptor registry now supports AF_INET/SOCK_STREAM
with protocol 0/6, blocking/nonblocking connect and I/O, send/recv/read/write,
MSG_PEEK/DONTWAIT/NOSIGNAL, dup/fcntl, poll, level-triggered epoll, FIONREAD,
shutdown, getsockname/getpeername and selected SOL_SOCKET getters (SO_ERROR,
SO_TYPE, SO_SNDBUF, SO_RCVBUF). Unsupported domains/types/options/flags fail;
setsockopt, bind/listen/accept, UDP and IPv6 are not implemented by this slice.
Address getters return the full IPv4 address size while copying only the
supplied capacity; unbound sockets report zero local address/port. Integer
SO getters copy a truncated prefix and return its copied length, including
zero capacity. The behavior was checked against the pinned musl layout and
[Linux v6.12's getter implementation](https://github.com/torvalds/linux/blob/v6.12/net/core/sock.c#L1976).

TCP callbacks increment/wake both object and descriptor-registry sequences
through validated process page tables. epoll now waits on the registry sequence
too, so NIC progress wakes mixed descriptor waits without runtime busy polling.
Reading readiness before atomic wait retains the existing lost-wakeup defense.

Open descriptor count is distinct from borrowed/epoll references. Both dup paths
increase it; last-descriptor close invalidates the backend handle, while closing
only a waited-on descriptor cancels that operation without killing its duplicate.
Closed-but-flushing streams retain no user notification addresses, finish queued
data before FIN, and have bounded linger. Process stop/fault/exit releases every
owned stream, including those linger states, before user page reclamation.
Reconfiguration refuses to invalidate a live stream's local tuple.

Socket read/write timeout options, resolver integration, further engine-needed
options, asynchronous DNS and actual Cobalt TLS integration are still required.
Zero network imports missing is not claimed. SIGPIPE delivery remains unavailable
along with the previously documented signal-delivery gap.

## Evidence

- tests/test_tcp_state.py compiles the actual net/tcp.c under ASan/UBSan with a
  deterministic packet boundary. Covers tuple/owner/generation isolation,
  partial ACK/retransmission, wraparound, duplicates/out-of-order, bounded/zero
  window, FIN/RST, SYN timeout, pool exhaustion and allocation teardown. This is
  host state-machine evidence, not a MyOS/Cobalt acceptance test.
- tests/test_elf_tcp.py runs an actual Ring 3 ELF in MyOS/QEMU. Two fixture
  servers require both simultaneous connections before replying. Two native
  pthreads independently send 32768 patterned bytes, shutdown write, receive
  20000 patterned bytes and observe EOF. Tests nonblocking connect/SO_ERROR,
  epoll timeout/network wake, poll, FIONREAD/peek, distinct local tuples, name
  buffer canaries, dup/close, refused ports, blocked-reader descriptor close,
  slot recycling, exhausted pool and no-NIC rejection. The complete session
  repeats; forced cancellation with a blocked worker repeats. The shell's
  page/heap reclamation counters return to baseline on each launch/stop.
- Scoped TCP tests passed at 00:17 +07 on 2026-10-01; native legacy HTTPS and
  network tests passed at 00:22/00:23. Normal kernel after full regression:
  241752 bytes, boot image 1474560 bytes.
- Two full runs stopped at the legacy initial-video frame. A stronger test
  confirmed black pixels in actual guest VRAM, not just delayed QEMU refresh.
  Root cause: the fixed SB16 DMA buffer 0x70000..0x73fff overlapped grown BSS;
  self-test images placed graphics state in that interval. BIOS SB16 now uses
  0x90000..0x93fff, separate from kernel/BSS (<0x80000) and initial stack
  0x80000..0x8ffff. Extending UEFI's fixed low-memory reservation failed in
  OVMF; the loader instead allocates an owned 64 KiB initial stack and an owned
  DMA allocation below the ISA 16 MiB ceiling. Boot ABI v2 passes its aligned
  16 KiB DMA subrange to the driver. EFI memory-map ownership excludes both
  allocations from PMM; the driver validates the DMA boundary and stack overlap.
  Shared C constants/static assertions and audio graphics-state integrity
  checks guard the layout. Self-test-image video/audio passed at 00:46/00:47;
  packaged UEFI DMA/framebuffer and network checks passed at 00:54/00:55 +07.
  Full tests/test_system.py passed at 01:28 +07 and restored the normal image,
  including concurrent native TCP, lifecycle/teardown, storage, memory/fault and
  VGA fallback coverage. No media feature or Phase 33.3 work was added. TCP
  tests are included in all/apps suites.
- Final packaged UEFI DMA test passed at 01:29 +07, including actual captured
  435/655 Hz PCM along with frame pacing, guest pixels and graphics integrity.
- Pinned Cobalt-only staging passed at 01:30 +07: 149 strong imports resolved,
  93 remain missing, weak TLS zero binding, relocation/RELRO and full staging
  reclamation. The 64 MiB OOM rollback leaves shell/apps usable. The engine was
  not executed; no HTML/JavaScript/DOM/input/HTTPS acceptance is claimed.

The five newly fixture-covered exports bring the import register to 149/242
strong symbols, with 93 remaining plus the weak TLS initializer. The register
records supported fixture subsets, not complete POSIX or browser compatibility.
