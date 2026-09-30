# Phase 33.1 system and optional-contract slice

Status: implemented and fixture-tested dependency work. Phase 33.1 and 33.2
remain open until native Cobalt passes the HTML/JavaScript/DOM/input/HTTPS gate.

The Ring 3 runtime now reports real MyOS process and scheduler thread IDs,
single-CPU affinity, root uid/euid, supported auxv values, bounded resource
limits, supported sysconf values and MyOS uname identity. Unsupported scheduler
and priority policies fail with errno rather than reporting false success.
`prctl` supports current-thread set/get name. Unknown auxv, sysconf, prctl,
resource and scheduler requests are rejected.

The imported configuration data exports are tied to implementation limits:
31-byte file names, 127-byte paths, `/` separator and eight process threads.
DRM creation, microphone discovery/creation and speech synthesis consistently
report unavailable; their dependent operations do not manufacture handles,
sessions, samples or capability success.

Validation:

- `python3 tests/test_elf_dynamic.py`
- `python3 tests/test_elf_dynamic.py --cobalt build/engine-evaluation/libcobalt.so`
- `python3 tools/check_cobalt_contract.py --library ... --headers ...`

The pinned Cobalt image resolves 108 tested strong imports and safely stops on
134 missing strong imports; the separate weak TLS initializer remains zero.
Static resolution is not native-engine acceptance.
