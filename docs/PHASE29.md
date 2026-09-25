# Phase 29 — HTTPS text browser and web search (QEMU baseline complete)

MyOS Desktop has a **kernel-resident, bounded text browser**. Its selected
engine is the in-tree HTML-to-text/link renderer in `gui/browser.c`; this is a
deliberate non-JavaScript browser profile, not a port of NetSurf or a modern
CSS/DOM browser. It connects the Phase 28 native TLS API to the GUI and
uses [DuckDuckGo Lite's documented non-JavaScript service](https://safe.duckduckgo.com/duckduckgo-help-pages/features/non-javascript/)
for real search results. Search and HTTPS page requests are made by MyOS itself inside QEMU over
certificate-verified TLS. Local HTTP pages remain available for testing.

## Try it

```bash
cd ~/myos
python3 tools/run_logged.py -- bash build.sh
qemu-system-x86_64 -m 64M -cpu max -nic user,model=rtl8139 \
  -drive format=raw,file=build/myos-current.img,if=floppy -boot order=a
```

At `myos>`, type `desktop`, then press **7** or click **Browser**. Enter
`valid-isrgrootx1 letsencrypt` and press Enter. The results page should show
`Search results (verified HTTPS)`; click its Let's Encrypt test-site result
to load the real HTTPS page. The **<** button returns to a cached search page.
You can also type `https://valid-isrgrootx1.letsencrypt.org/` directly.
Up/Down scroll; Esc closes the desktop. A bare domain defaults to HTTPS.
The GUI requests a DHCP lease when needed.

For a local HTTP fixture, run `python3 -m http.server 8000 --bind 127.0.0.1`
in WSL and visit `http://10.0.2.2:8000/` in MyOS.

## Supported behavior and limits

The renderer handles bounded plain text and a small HTML subset: title,
headings, paragraphs, line breaks, lists, basic entities and clickable
absolute/relative links. DuckDuckGo Lite result links are decoded into direct
target URLs. Back restores up to eight page snapshots without a new network
request. URL length is limited to 191 bytes, response collection to 64 KiB,
and layout to 160 lines; oversized pages fail visibly and recovery is tested.
At least 16 MiB RAM was exercised in QEMU. The browser runs in **kernel mode**,
not in an isolated user process.

Certificate and hostname verification are mandatory for HTTPS. The small
trust store and TLS 1.2/cipher restrictions in [PHASE28.md](PHASE28.md) still
apply; many otherwise valid websites can be rejected. HTTP redirects are
shown but not followed. CSS, images, forms, JavaScript, cookies, tabs,
non-ASCII text rendering and media playback are not supported. This profile
cannot render modern interactive sites or YouTube; Phase 32 retains the
separate online video acceptance gate. A site changing its Lite markup,
certificate chain or rate limits may affect live search.

## Validation

```bash
python3 tools/run_logged.py -- python3 tests/test_browser.py
python3 tools/run_logged.py -- python3 tests/test_browser_https.py
python3 tools/run_logged.py -- python3 tests/test_browser_live.py
python3 tools/run_logged.py -- python3 tests/test_https.py
python3 tools/run_logged.py -- python3 tests/test_network.py
python3 tools/run_logged.py -- python3 tests/test_system.py
python3 tools/run_logged.py -- python3 tools/release.py build
python3 tools/run_logged.py -- python3 tests/test_browser_packages.py
python3 tools/run_logged.py -- bash build.sh
```

The local GUI suite exercises PS/2 keyboard/mouse input, HTTP parsing,
navigation and recovery. The HTTPS GUI suite uses a local test CA to verify
page/link/Back behavior and rejects a wrong-host certificate at 16 MiB.
The live test on 2026-09-26 searched DuckDuckGo Lite over MyOS TLS, clicked
a real result, loaded its verified HTTPS page, then returned to cached search
results. It requires the external search provider and target site to remain
reachable and compatible. The system suite protects the existing boot,
desktop, app, storage and network foundations. Packaged BIOS and UEFI
images also boot and load a local page. Commands are recorded in
`build/activity.log` for `tail -f build/activity.log`. The owner pushes
commits manually.
