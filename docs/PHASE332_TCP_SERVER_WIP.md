# Phase 33.1/33.2 — owned TCP server scope

Updated 2026-10-05 15:38 +07. Source baseline3cbd25f, main unchanged.
Private copied mutable tree /tmp/myos-tcp-server-1Hd4dq. No33.3/push/closure.

## Production checkpoint — 16:37

Main pipeline1701 terminal0 PASS16:31:49: mandatory full test_system.py default
suite, normal UEFI restoration, native server BIOS/UEFI and actual paired
core/provider loader BIOS/UEFI. Production serial missing28/provider0/weak1,
214/24288.4%; registry212/24287.6%. Paired checks are NOT Cobalt web execution.
Native outputs tcp-server-y1wzhdco and tcp-server-6o886ngs; paired outputs
pair-kmwynu06 and pair-iobh77r3. No live main image owner after pipeline ended.
Normal SHA256:
- kernel.bin: 5efa610fccbe72e00667487280462920da374f4142006313daa64ed79230e56b
- platform-runtime.bin: 16f213fb1e9a7bd2fb1268587b8236a30bab79b681f1ec7a03beb7ce09c1d4a1
- platform-runtime.mrt: cecea42146f74c93699ff335cf8e95ef0569e8d6488f70e1e922b65d1a6efb31
- platform-runtime.manifest.json: f067cdc0d8e33bde68afa99ebe7db50b462bfa39d4131d5dedaf04428690e08a
- myos-current.img: 5e7145f48789acbc49fd2d86eec5ab4984ee24502e1970d84e092269595967ae
- myos-uefi.img: d8a451614d7d1b70cf75d8aa7773566d27b02f8a542423773fce0ba05d371ff2

## Promotion history — 15:43 (superseded by terminal proof above)

Actual private paired10184 terminal0 15:40:52, BIOS pair-_lbsg_s0 and UEFI
pair-3c1i2b23 PASS,214/24288.4%,28missing/provider0/weak1. Main promoted
15:43:04 via integrate_server.sh, mandatory full+normal UEFI+server/pair both
profiles1701 LIVE. Freeze main source/tests/images until terminal. Full results
in build/tcp-server-full.log; final scopes tcp-server-{native,pair}-{bios,uefi}.
No new full/main pair result claimed yet. Registry212/24287.6% scoped coverage.
Origin/main now3cbd25f fetched15:42:43, owner pushed earlier checkpoints.
Agent never pushes. Source uncommitted until required pipeline passes.

## Implemented contract

- Additive SYS_TCP_BIND/LISTEN/ACCEPT53/54/55, checked copy-in/out and notification
  ownership; request24 bytes. Stream generation/owner validation remains shared.
- IPv4 stream wildcard/configured local address binding; port0 ephemeral;
  duplicate bindings EADDRINUSE, foreign local address EADDRNOTAVAIL. Native
  HTTP/TLS ephemeral allocation now skips explicitly bound user pool ports;
  bind rejects an active legacy client port. No REUSEADDR/UDP/IPv6 claim.
- Bounded listen backlog1..31 (0 becomes1); 32 total transports including
  listeners/accepted/pending/closing. Real SYN/SYNACK/ACK, three bounded retries,
  pending allocation rollback, FIFO completed handshakes, poll/epoll readiness.
- accept pins original descriptor generation across wait; NONBLOCK and shared
  SO_RCVTIMEO deadline, EINTR, close/reuse EBADF. State->object lock ordering.
  EMFILE checked before dequeue. New accepted descriptors do not inherit
  NONBLOCK/CLOEXEC; endpoint timeouts inherited; peer output safely truncated.
- Listener close destroys unaccepted children; accepted streams independently
  owned. Owner fault/cancel releases all child buffers and user notification
  references. Existing bounded close-flush tuple lease still applies.
- Bad user output pointers may fault the isolated caller, as existing name
  adapters; no claim of complete POSIX EFAULT handling. No fake success handles.

## Evidence

- Private kernel+runtime build6654 terminal0 15:21:42. Kernel PROCESS_TESTS1
  242912 bytes (475 sectors), linker guards passed. Main normal images untouched.
- First failures were test harness issues: F_GETFL expected0 instead of O_RDWR2;
  peer did not acknowledge completion before process-exit cancelled queued data;
  same-port rebind attempted during existing bounded closing tuple lease.
  Captured SYN/ACK and guest exit19/28 retained. Fixed expectations/protocol,
  not transport success criteria. Test now verifies all8192 echo bytes.
- BIOS34094 terminal0 15:29:40 three relaunches, two real host peers, binary
 4096 bytes each, EMFILE pending preservation, truncation, alias listener close,
  accepted-child independence, EOF/halfclose, blocked close/reuse and Escape;
  exact pages/heap90624 before/after. tcp-server-uz_m_023.
- UEFI scope94730 passed server three launches and socket-options regression;
  helper then exited2 because obsolete test_message_batch.py filename, corrected
  to actual test_message.py. No full-pipeline PASS claimed for94730.
- Extended37664 terminal0 15:33:08: BIOS/UEFI server with epoll data/readiness,
  targeted SIGUSR1 interruption ->EINTR and handler exactly once, plus message
  and native concurrent client TCP regressions. Outputs tcp-server-xqnrb71w,
  tcp-server-q975f8lt, message-05n_ow20. Socket-options passed preceding94730.
- ASAN/UBSAN state machine terminal0 15:34:32: legacy-port bind conflict,
  foreign address, stale/foreign handles, backlog rejection, allocation failure,
  bad ACKs, final ACK with payload, invalid notification before dequeue,
  child surviving listener close, half-open timeout and listener child teardown.
- Actual pinned core/provider paired10184 terminal0 15:40:52, observed missing28
  in two BIOS core probes15:38:56, provider0, weak-zero1; 214/24288.4%.
  Prior production211/24231missing; new main actual proof pending1701.

## Next

Finish paired10184, promote reviewed sources+native/unit tests, run mandatory
full system regression and restore normal BIOS/UEFI; main scoped server/pair
both profiles; docs/local checkpoint only after required results. Remaining
after these3: file11, audio output/config2, player13, decode target2. Native
Cobalt HTML/JS/DOM/input/verified HTTPS/endurance/resources still0/5.

Windows recovery helpers: build_server_probe.sh, run_server_probe.sh,
run_server_uefi.sh, run_server_more.sh, run_server_unit.sh, run_server_pair.sh.
All repo operations logged through tools/run_logged.py. Mutable-source copies,
not symlinked main source; separate QMP/image/asset paths prevent collisions.
