# Public TLS test fixtures

The private keys test-server.key and rsa-server.key are intentionally public
fixtures for local HTTPS regression servers. They are not production secrets.
Never use these keys/certificates to authenticate a deployed service or trust
this test CA on a real system. Tests explicitly load these keys; see
 tests/test_https.py, tests/test_browser_https.py,
 tests/test_phase32_stream.py and tests/test_phase33_online_video.py.
The normal build must continue to reject the test root (docs/PHASE28.md).
These fixtures are retained to avoid breaking reproducible negative TLS tests.
