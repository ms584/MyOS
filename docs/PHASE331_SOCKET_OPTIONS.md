# Phase 33.1 socket timeouts and option ownership

Dependency checkpoint after a6e2899, not Phase 33.1/33.2 closure. Cobalt has
not executed HTML/JavaScript/DOM/input/verified HTTPS. No Phase 33.3 work.

## Boundary and supported behavior

The pinned [Cobalt wrapper](https://github.com/youtube/cobalt/blob/3befd332486952f6603ae3baf2a8713878d6e200/starboard/shared/modular/starboard_layer_posix_socket_abi_wrappers.cc#L443)
passes setsockopt's level, option, byte pointer and socklen through unchanged.
MyOS now implements SOL_SOCKET SO_RCVTIMEO (20) and SO_SNDTIMEO (21), using the
x64 16-byte timeval: two signed 64-bit seconds/microseconds fields, not timespec.
This is supported-subset coverage for setsockopt, not every socket option.

- Timeouts belong to the open socket description and are shared by dup/fcntl.
  The two socketpair endpoints have independent receive/send options.
- Nonzero positive values round upward to the 100 Hz PIT tick. Zero disables
  the I/O deadline. Negative seconds select an immediate timeout; getters then
  report zero, matching the inspected Linux socket behavior. Invalid usec
  (<0 or >=1000000) returns EDOM without changing the previous option. Huge
  positive seconds saturate to the infinite setting without multiplication
  overflow. Short inputs fail EINVAL; null sufficient-sized inputs fail EFAULT.
- recv/read and send/write capture one monotonic deadline at call entry, so
  repeated readiness wakes and later option changes cannot renew that deadline.
  Expiry without bytes returns -1/EAGAIN; partial I/O returns available/accepted
  bytes. EOF and a ready result are checked before deciding to time out.
- O_NONBLOCK and MSG_DONTWAIT override the wait. poll/epoll keep their own
  independent timeout parameters. Clock adjustments do not alter PIT deadlines.
- Blocking connect uses SO_SNDTIMEO. Expiry returns EINPROGRESS while the
  transport handshake continues; later poll/SO_ERROR observes completion or
  the real backend error. The timeout does not destroy the connection.
  Internal connect status reads the persistent transport failure, not the
  consumable SO_ERROR value: another thread clearing SO_ERROR cannot conceal
  a failed handshake and leave connect sleeping until its deadline.
- Generation validation still makes close/reuse cancel the operation with
  EBADF while another duplicate remains usable. Options introduce no extra
  mappings, threads or backend objects, and borrowed references remain bounded.
- getsockopt supports the two normalized timeval options, actual socket type,
  buffer capacities and SO_ERROR for TCP/socketpair. It copies only the supplied
  prefix and reports that copied size, preserving short-buffer canaries.

The [Linux v6.12 timeout setter/getter](https://github.com/torvalds/linux/blob/v6.12/net/core/sock.c#L401)
was inspected for validation, rounding, negative/infinite behavior and output
layout. This is ABI/behavior research, not a Linux implementation substituted
for native MyOS execution.

## Native evidence

tests/test_socket_options.py builds an actual Ring 3 ELF and runs twice in
MyOS/QEMU. The fixture checks endpoint/dup sharing, errno/output preservation,
usec/length/handle/type rejection, rounding, negative/overflow inputs,
nonblocking override, partial I/O, repeated wakes, option changes during a
captured wait, close/reused slots, duplicate survival, shutdown/EOF and cleanup.

For TCP, the fixture inserts QEMU's packet-buffer filter after a guest marker,
before the connect or send attempt. Real SYN/data packets are delayed, without
changing guest memory or fabricating syscall results. Native connect expires
around 80 ms and remains pending; removing the filter permits the same socket
to connect. The transmit ring fills while ACKs are delayed; native send expires
around 80 ms. Releasing packets drains every accepted byte unchanged, and EOF
is observed. Both page and heap counts return to baseline after each launch.
The fixture also consumes a refused connection's reported SO_ERROR, then
checks persistent failure through the actual Ring 3 syscall boundary;
reading it does not recreate or consume the public SO_ERROR value. ASan/UBSan
tests check the same distinction after reset and reject invalid modes.
The initial host-only receive-buffer limit did not reliably backpressure the
guest because QEMU/host TCP adds intermediate buffering; it was replaced with
the controlled real-packet stall rather than weakening the timeout assertion.

Scoped fixtures passed at 02:18 +07 on 2026-10-01 after the consumed-error fix.
The first new full run was intentionally interrupted at 02:12 for that fix,
and restored the normal image. It is not a full PASS. The replacement full run
passed at 02:49, including this fixture and runtime/lifecycle, persistence,
memory/fault recovery and VGA fallback. The normal image was restored:
kernel 242808 bytes, image 1474560 bytes. The pinned hash/import check passed
at 02:05: 150 tested strong import subsets, 92 remaining strong imports and a
separately tracked weak TLS symbol. Final Cobalt-only staging passed at 02:50:
150 resolved/92 missing, weak TLS zero, relocation/RELRO/reclamation and 64 MiB
OOM rollback. Packaged UEFI DMA/framebuffer/real PCM and network checks also
passed at 02:50. Cobalt itself was not executed.

## Remaining required network contracts

TCP_NODELAY, keepalive, buffer configuration and other engine-needed options
are not implemented by this slice; unsupported options return ENOPROTOOPT.
Resolver/interface APIs and actual Cobalt's TLS/HTTPS integration remain open.
Signal interruption/delivery remains unavailable. Timeout fixture PASS and
150 imports do not establish native Cobalt compatibility or web acceptance.
