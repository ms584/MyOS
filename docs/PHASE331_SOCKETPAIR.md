# Phase 33.1 local socket descriptor slice

This is a tested AF_UNIX stream dependency, not a TCP/resolver/TLS implementation
or native-engine acceptance. Phase 33.1/33.2 remain OPEN and 33.3 has not started.

## Ownership and supported behavior

- socketpair uses the pinned musl NONBLOCK/CLOEXEC values and accepts AF_UNIX,
  SOCK_STREAM and protocol zero only. Other domains/types/protocols fail.
- One refcounted object owns two independent 4096-byte receive rings. A slot's
  access tag identifies its endpoint; duplicates retain that endpoint.
  Status flags are per endpoint and shared by its duplicates, not its peer.
  CLOEXEC remains descriptor-local. Both slot installs are all-or-nothing.
- send/recv and read/write use the reference already acquired for the operation;
  they do not reacquire a reused descriptor number. Blocked operations check
  the slot generation, and close wakes them with EBADF. Closing one duplicate
  preserves the peer connection until the last duplicate closes.
- Full buffers block producers, empty buffers block consumers, and partial I/O
  is returned without a busy loop. DONTWAIT is per-call; PEEK does not consume.
  Data/space changes wake all waiters through the existing sequence/wait service.
- shutdown applies to the endpoint including its duplicates. Write shutdown
  preserves already queued data before peer EOF, and still allows reverse I/O.
  Read shutdown discards that receive queue and makes peer writes fail EPIPE.
- poll/level-triggered epoll expose data, space, EOF, RDHUP and HUP. An immediate
  EPIPE write is writable-ready rather than leaving a producer asleep forever.
  FIONREAD reports the selected endpoint's queued bytes. Registrations are
  removed on descriptor close so slot reuse cannot attach a stale event.

## Evidence

user/elf-platform-socketpair.c executes in Ring 3 at 16 MiB through
tests/test_elf_dynamic.py. It covers duplex transfer, duplicate flags/lifetime,
PEEK, DONTWAIT, full-buffer readiness, half/full shutdown and EOF, poll/epoll
caller data, FIONREAD, close during read and write, duplicate survival, descriptor
reuse, blocked-writer wake, two waiting readers, partial-install rollback,
and 64 repeated pair creations/closures. Process exit checks page reclamation.
The file/fsync/reboot and existing pthread/wait fixtures remain in the same run.

The pinned hash/register check passes with 139 fixture-covered strong imports,
103 strong imports still missing and a separately tracked weak TLS initializer.
Updated Cobalt staging passed at 256 MiB (139 resolved/103 missing; all staged
pages reclaimed) and 64 MiB (OOM rollback). Full tests/test_system.py passed at
19:38 +07 on 2026-09-30 and restored the normal framebuffer image; the suite
now includes tests/test_run_logged.py. Missing imports still block execution.

## Explicit limits and next dependency

No network socket(), bind/listen/accept/connect, addresses/options, resolver,
datagrams, ancillary messages, WAITALL/OOB flags or asynchronous SIGPIPE
delivery is supplied here. MSG_NOSIGNAL has the tested no-signal EPIPE behavior.
Supported send/recv/shutdown bindings currently operate on local pairs only;
the CSV records fixture coverage, not full socket or engine compatibility.
The existing 32 descriptor slots and fixed runtime allocator bounds still apply.

The legacy kernel TCP path in net/net.c owns one tcp_active/tcp_peer/source_port,
one sequence/acknowledgement pair and one 8192-byte stream ring. Native browser
HTTPS uses that serialized path. Wrapping it under several descriptor numbers
would alias connections and does not implement Cobalt concurrent requests.

The network follow-on must introduce independent connection state, stable
generation-tagged handles, source-port demultiplexing, bounded send/receive
queues, asynchronous NIC/timer progress and close/cancel wake ownership. Each
connection needs sequence/ACK, EOF/error and timeout state; shared frame buffers
must remain serialized while the protocol state is independent. Preserve the
legacy browser baseline through a compatibility adapter. Only then expose
user socket/connect/send/recv/options, readiness and resolver contracts.

Test two simultaneous scripted TCP peers, partial writes/reads, backpressure,
refused/timeout/reset/FIN, descriptor reuse, close with blocked workers and
reconnect. Follow with hostname/chain/time-failing HTTPS and concurrent external
script/asset requests inside MyOS. Host fixture servers are not engine playback.
