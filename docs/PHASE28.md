# Phase 28 — HTTPS platform (QEMU baseline)

MyOS can now fetch an HTTPS resource from the Shell with native TLS 1.2. The
implementation uses vendored Mbed TLS 3.6.7 LTS, with SNI, peer certificate and
hostname verification required. It never falls back to an unverified session.

## Use

Build and start QEMU with an RTL8139 NIC and a CPU that exposes RDRAND:

```bash
bash build.sh
qemu-system-x86_64 -m 64M -cpu max -nic user,model=rtl8139 \
  -drive format=raw,file=build/myos-current.img,if=floppy -boot order=a
```

In the MyOS Shell:

```text
security
net up
net https HOST 443 /PATH
```

`security` reports the RTC-derived UTC Unix time and hardware entropy status.
HTTPS refuses to start if the RTC date is implausible or RDRAND is unavailable.
The RTC must represent UTC. The client has a fixed 64 KiB response limit and a
single TCP connection at a time; it is intended as a platform API, not a
general web browser. Only TLS 1.2 ECDHE-ECDSA/RSA with AES-128-GCM-SHA256 and
SHA-256/384 certificate chains are configured. TLS 1.3, IPv6, redirects,
chunked decoding and certificate revocation checks are not supported yet.

The production trust store currently contains ISRG Root X1 and DigiCert
Global Root G2 DER certificates sourced from Ubuntu's Mozilla CA package.
Other valid sites may be rejected if their chain terminates at a different
root or uses an unsupported algorithm. Test builds (`TEST_CASE=28`) add a
separate test root; the normal image does not contain it. Test-only server
keys under `security/certs/` have no production use.

## Validation

```bash
python3 tools/run_logged.py -- python3 tests/test_security.py
python3 tools/run_logged.py -- python3 tests/test_https.py
python3 tools/run_logged.py -- python3 tests/test_network.py
python3 tools/run_logged.py -- python3 tests/test_system.py
```

The HTTPS suite boots QEMU against a local TLS server and checks valid
ECDSA and RSA connections, then rejects wrong-host, untrusted and expired certificates,
invalid RTC time and missing hardware entropy. It also proves that the
production build rejects the test root. The suite rebuilds the normal
framebuffer image when it finishes. On 2026-09-25, the normal image also
fetched `https://valid-isrgrootx1.letsencrypt.org/` directly in QEMU and
reported `HTTPS verified`; this live smoke test depends on outside network
availability and the site certificate chain, so it is not a required test.

## Source and license

`vendor/mbedtls/` is Mbed TLS 3.6.7 LTS from the official release archive
`mbedtls-3.6.7.tar.bz2` (SHA-256
`a7e8bcbec0e6f761b4af24f25677626b35f762f68eef79c08677a363212d11f6`).
It retains the upstream dual Apache-2.0/GPL-2.0-or-later license in
`vendor/mbedtls/LICENSE`. MyOS changes its configuration and supplies
freestanding memory, time, entropy and network callbacks.

The Phase 29 Browser window now uses this verified HTTPS API for text pages
and non-JavaScript search. HTTPS support does not imply JavaScript, modern
web compatibility or YouTube playback.
