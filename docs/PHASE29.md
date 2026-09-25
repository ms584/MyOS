# Phase 29 — Browser preview (in progress)

The MyOS desktop now has a **kernel-resident HTTP browser preview**. It is a
working integration of the Phase 27 network client and the graphical desktop,
but **Phase 29 is not complete**. Phase 28's native TLS, trusted time,
entropy and certificate validation do not exist yet, so the browser cannot
open HTTPS search results. Do not describe this preview as a full browser,
a NetSurf port, or evidence that YouTube works.

## Try the current preview

In one WSL terminal, serve a local page or directory:

```bash
cd ~/myos
python3 -m http.server 8000 --bind 127.0.0.1
```

In another WSL terminal:

```bash
cd ~/myos
python3 tools/run_logged.py -- bash build.sh
qemu-system-x86_64 -m 64M \
  -drive format=raw,file=build/myos-current.img,if=floppy -boot order=a \
  -nic user,model=rtl8139
```

At `myos>`, type `desktop`, press **7** or click **Browser**, then type
`http://10.0.2.2:8000/` in the address field and press Enter or click Go.
Click a listed link to navigate; click `<` to go back. Up/Down scrolls the
page. Clicking the address field clears it for another URL. Esc closes the
desktop. The window automatically requests a DHCP lease when needed.

The preview renders bounded plain text and a small HTML subset: title,
headings, paragraphs, line breaks, lists, entities and clickable links.
Absolute and relative HTTP links work. A 32 KiB response buffer and 160
laid-out lines bound memory use. Oversized responses fail visibly, and another
page can be loaded afterward. Unsupported HTTPS URLs, HTTPS redirects and
search queries display an explicit Phase 28 message. A missing NIC also
produces a visible error. CSS, images, forms, JavaScript, cookies, HTTP
redirect following, multi-tab browsing and user-process isolation are absent.
Text rendering currently uses the built-in ASCII font; non-ASCII page text
is replaced with `?`. This preview is unsuitable for Thai-language pages.
The built-in browser runs in the kernel alongside the existing GUI apps.

## Engine and search decision still open

The vendored NetSurf `libwapcaplet` tested in Phase 26 is only one dependency,
not an HTML/CSS browser engine. [NetSurf's project overview](https://www.netsurf-browser.org/)
lists its parser, DOM, CSS and other libraries; its
[build instructions](https://github.com/netsurf-browser/netsurf/blob/master/docs/env.sh)
show the additional porting work. NetSurf is a candidate for a richer
document browser, but its own [JavaScript status](https://www.netsurf-browser.org/documentation/info)
describes incomplete support, so choosing it would not establish Phase 32's
YouTube goal. No final engine has been selected or ported in this phase.

For real web search after Phase 28, [DuckDuckGo documents HTML and Lite
non-JavaScript versions](https://safe.duckduckgo.com/duckduckgo-help-pages/features/non-javascript/).
The exact provider page and compatibility must be validated in MyOS over
certificate-verified HTTPS. The current browser does not fetch a search
provider, and host-side browsers or TLS proxies do not count as MyOS search.

## Validation

```bash
python3 tools/run_logged.py -- python3 tests/test_browser.py
python3 tools/run_logged.py -- python3 tests/test_system.py
python3 tools/run_logged.py -- python3 tests/test_network.py
python3 tools/run_logged.py -- python3 tools/release.py build
python3 tools/run_logged.py -- python3 tests/test_browser_packages.py
```

The browser test uses real QEMU PS/2 keyboard and mouse events, local HTTP
fixtures, framebuffer inspection and parser-state checks. It covers the URL
field, parsed HTML/hidden script and style text, links, Back, entities,
oversized-response recovery, HTTPS redirect/search limits, no-NIC behavior,
desktop cleanup and a 16 MiB RAM boot. The system suite regresses the
existing desktop, apps, boot modes and storage. Commands are recorded in
`build/activity.log` for the owner to watch with `tail -f build/activity.log`.
The owner pushes GitHub commits manually.
