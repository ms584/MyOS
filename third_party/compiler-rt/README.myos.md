# compiler-rt emulated TLS contract fixture

Unmodified `emutls.c` from LLVM tag `llvmorg-18.1.8`:
https://github.com/llvm/llvm-project/blob/llvmorg-18.1.8/compiler-rt/lib/builtins/emutls.c
SHA256: a4437f3e329a0835adbc7df38e36df12f358d05c073b11813c409f849c2345d2

LICENSE.TXT is copied from that tag's root (Apache-2.0 WITH LLVM-exception).
SHA256: 8d85c1057d742e597985c7d4e6320b015a9139385cff4cbae06ffc0ebe89afee

Only native test fixtures include this file. It is NOT a production MyOS
export, a substitute for Cobalt's own in-image implementation, or a claim of
compiler/toolchain equivalence. Test-only `tests/emutls-abi` headers adapt the
POSIX calls to the pinned Cobalt pointer-sized key and opaque object sizes.
The fixture inspects private state in the same translation unit to verify
growth, allocation mappings and destructor cleanup without modifying upstream.
No build/test fetches network dependencies; the source and license are vendored.
