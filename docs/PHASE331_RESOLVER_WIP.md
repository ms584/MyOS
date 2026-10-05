# Phase 33.1: native resolver — tested subset and full regression checkpoint

## Current authoritative status — 2026-10-05 08:20 +07

Installed/tested local checkpoint in /home/xytron404/myos after465b68b (see Git HEAD).
Normal BIOS build253144 bytes. Actual SYS_DNS52 START/STATUS/CLOSE/CONFIGURED,
kernel timer progress, scheduler cleanup and three resident exports are installed.
Local byte/string helpers fixed resident undefined libc symbols; wire parser uses
explicit freestanding memcpy/memcmp declarations, not fortified host wrappers.

- Native BIOS21536 exit0 08:10:54: ABI validation, ownership/stale handles,
  pool bounds, multi-A/CNAME/wrong-ID+question, truncated UDP-to-TCP fallback,
  eight concurrent workers, timeout progress, worker-exit cleanup, forced cancel,
  relaunch/exact heap/pages PASS. build/resolver-native.log, dns-z9c3f_4l.
- Offline/no-entropy/UEFI27945 exit0 08:13:23 PASS; logs resolver-offline.log,
  resolver-no-entropy.log, resolver-uefi.log; UEFI artifact dns-4vr3dxbc.
- Host wire/job ASan+UBSan PASS08:19:40: truncations/mutations/hostile packets,
  ownership/generation/pool/timeout/cleanup and fragmented TCP framing.
- Paired binding44021 PASS:52 missing; build/resolver-pair.log. NOT execution.
  Fixture-tested188/24277.7%; paired bindings190/24278.5%. Not complete API ABI.
- Full architecture79950 exit0 08:55:30: default ALL test_system.py PASS,
  including DNS/packing/fault variants/low RAM/VGA. Normal BIOS/UEFI restored;
  build/resolver-full-regression.log and resolver-final-uefi-build.log.
  Terminal; no production image owner. Normal runtime33888/packed27850,
  kernel253144/BSS end0x7d450; test kernel261336/cap262144 leaves808 bytes.

Final SHA256 (normal BIOS/UEFI after full):
```
kernel.bin d5f2aab05d62b5f09227ae9e4702c6f3ded3165f6fe2c24146f520725c3aea2e
platform-runtime.bin 56e6aef8455a97026b5d8b83379e1be44acb636c3dcb1a9cdb2fbb1c65bbeec6
platform-runtime.mrt 259e80f1b9f137b49c220563b6aa78f0db865fe60aaef0cd9b9a06b32926066a
myos-current.img 5c5b4220e37ac48438502f8c809441839ccbf55cdf84b66458fb0f99f5727ed7
myos-uefi.img afb12dbc9ac0aa639304f63dde58201aa7d70f8751e2479cd9bedcd8b72ad05f
```

Limits: IPv4 subset, classic512-byte DNS/no EDNS, up to8 addresses, fixed built-in
services. IPv6/CNAME-only follow-up and full service/flags behavior remain open.
Nonzero pthread_kill is still EINVAL: initial native signal-interruption test
failed and evidence is retained in build/resolver-native-signal-gap.log.
Current fixture proves worker-exit kernel cleanup, NOT cross-thread signal/EINTR.
Network peer is deterministic Ethernet fixture, NOT Internet or Cobalt proof.
Actual Cobalt0/5, Phase33.1/33.2 OPEN, no33.3. No push.

## Historical prototype notes (superseded where contradicted above)

2026-10-05 +07, prepared while runtime-packing full61037 used shared sources.
Full61037 now exit0 06:57:27, normal BIOS/UEFI restored/hashes recorded in
PHASE332_RUNTIME_PACK.md. Finish its local checkpoint before integrating DNS.
Keep the full Cobalt objective OPEN.
No new imports count is claimed. This is not Phase 33.3.

## Staged implementation and proof

Windows workspace work directory:
`C:/Users/Xytron/Documents/Codex/2026-09-30/myos-checkpoint-9f58024-home-xytron404-myos-2/work/`

- `dns-wire.{h,c}`, `dns-wire-check.c`: A/IN query builder and bounded full-response
  parser. Labels/names, case folding, backward compression with bounded traversal,
  question/type/class/ID matching, CNAME chain, duplicate removal, all sections
  bounds, 8-address limit and EAI errors. Output untouched on failure.
  Consult [RFC1035](https://www.rfc-editor.org/rfc/rfc1035.html) sections 2.3.4,
  4.1.1–4.1.4 and 4.2. The parser signals TC for TCP fallback; not DNSSEC validation.
- `dns.{h,c}`, `dns-call.h`, `dns-job-check.c`: nonblocking request pool16,
  process+thread owner, generation-checked handles, copied query/server, secure
  RDRAND transaction/source-port selection, retries/global timeout, user wait-word
  notification through process_notify. Partial TCP length/query/reply framing
  reuses existing owned stream pool. Job owns a bounded heap buffer; cleanup
  closes internal TCP handle and frees the job. No blocking sti/hlt in INT80.
- `netdb.h`, `platform-netdb.c`: selected LP64 musl1.2.6 declarations (addrinfo48,
  sockaddr16; pointer offset24; negative EAI values), getaddrinfo/freeaddrinfo/
  gai_strerror adapter, numeric IPv4 legacy forms, numeric/fixed http/https/domain
  service lookup, flags/family/type checks, per-call stack wait word, error cleanup,
  independently allocated addrinfo chain. No mutable resident-runtime globals.
  Private SYS52 proposed: START0/STATUS1/CLOSE2/CONFIGURED3. Not installed.
- musl archive: build/math-evaluation/musl-1.2.6.tar.gz, SHA256
  d585fd3b613c66151fc3249e8ed44f77020cb5e6c1e635a616d3f9f82460512a.
  Read exact include/netdb.h, src/network/{getaddrinfo,lookup_serv,inet_aton}.c
  before promoting supported semantics. The shim is selected declarations, NOT
  a verbatim complete upstream header or proof of general resolver conformance.

Host evidence (run_logged.py activity timestamps; binaries in /tmp):

- 06:30:36 `/tmp/myos-dns-wire-check-20261005` ASan/UBSan PASS: A/dedup/CNAME,
  compression loops/out-of-range/RDATA, question/ID, RCODE/TC, atomic output,
  every truncation/bit mutation, max wire-name boundaries and 20000 hostile frames.
- 06:38:46 `/tmp/myos-dns-job-check-20261005` ASan/UBSan mocked transport PASS:
  foreign owner/thread rejection, stale handle, pool saturation, unique ports,
  reverse completion16, timeout, thread/owner cleanup, allocation/no-entropy/no-NIC
  errors, partial TCP fallback/framing and balanced job allocations.
- Freestanding target compile of dns.c: 1737 text/0data/256bss bytes BEFORE
  syscall/net/timer/cleanup hooks; not final kernel growth. netdb adapter target
  object2122 text/0data/0bss. Exact current headers copied into staging before
  ownership mock test. GCC builtin headers path actually /usr/lib/gcc/x86_64-linux-gnu/15/include.
- Host/mock tests do NOT prove NIC, syscall validation, interrupts, signals,
  scheduling, process teardown, native DNS, Cobalt or external HTTPS.
- Host mock extended 06:58:00: unmapped wait-word notification failure wakes
  without dereferencing invalid memory; still mocked, not native proof.
- `dns_network_fixture.py` and `probe_dns_network.py`: private Ethernet DHCP/
  ARP/UDP/DNS test peer with socket NIC. 06:55:19 native unchanged MyOS legacy
  resolver smoke PASS; `/tmp/myos-dns-network-ahwe8bn0/serial.log`. Normal image
  frozen from pack-an52e4tw, no shared-image changes. Legacy wrong-ID-first
  lookup fails (expected old limitation), good multi-A/CNAME answers resolve.
  This proves fixture wiring, NOT new resolver/Internet/Cobalt. TCP peer is
  prepared but not yet exercised natively. Shutdown reports peer errors.

## Required integration / next actions

1. Runtime packing full/normal restore passed; finish docs/local commit first.
2. Review staged code against CURRENT kernel (do not install stale work header
   mirrors). Copy ONLY new DNS/netdb files after review. Add driver/parser and
   typed resident netdb module to build; preserve boot/RX/TLS/runtime size caps.
3. net.c: dispatch validated UDP to job_receive BEFORE the legacy UDP filter;
   expose configured DNS + checked UDP send, and block DHCP reconfigure while
   live DNS requests exist. Do not call legacy net_resolve from Ring 3 syscall.
4. Timer tick: bounded RX/driver progress even with all callers asleep.
   Scheduler stop-thread and stop-process: release DNS before TCP teardown.
   Release based on process+full-width unique tid; no stale user wait-word writes.
5. SYS52: validate reserved fields, copied 264-byte start request/name terminator,
   aligned writable8-byte wait word, 32-bit handles and writable296-byte status
   before side effects. STATUS pending/errors leave output untouched. No direct
   user-VA accesses; owner/thread/generation checks authoritative.
6. Promote/review the staged native deterministic network fixture (VM currently
   only supports bool nic=>slirp). A test-only configurable QEMU socket NIC plus
   Ethernet DHCP/ARP/DNS/TCP server can prove real RTL8139/MyOS paths without
   changing global host DNS or pretending a Linux browser is Cobalt. Test numeric/
   NULL/default hints, flags/services/errors, concurrent/out-of-order answers,
   malformed/wrong peer/question/ID, TC/partial TCP, timeout, EINTR, worker exit,
   process cancel/relaunch, stale handles, readonly/bad buffers and exact resources.
7. Audit actual semantic gaps: no IPv6, only fixed named services, no hosts/NSS
   database, no DNSSEC/cache/search list, no CNAME-only follow-up query yet,
   classic512-byte response bound/no EDNS. Do NOT label full APIs complete from
   these prototypes or silent fabricated answers; complete required real behavior.
8. After native fixtures, add3 exports/import register, full architectural tests,
   BIOS/UEFI normal restore/hashes, truthful docs/local commit. Owner pushes only.
   Then remaining network/files/media capability dependencies and actual Cobalt
   HTML+JS/DOM+input+verified HTTPS+>=60s/close/relaunch gates. Still0/5.
