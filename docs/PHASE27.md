# Phase 27 — QEMU network baseline

**Accepted in QEMU:** RTL8139 PCI I/O driver, bounded DMA rings, Ethernet,
IPv4, ARP, DHCP, DNS, UDP and a single TCP/HTTP client request. Network code
is separate from the device driver so later NICs can replace RTL8139.
This is not HTTPS or a browser. Real hardware remains untested.

## Try it

```bash
cd ~/myos
python3 tools/run_logged.py -- bash build.sh
qemu-system-x86_64 -m 64M \
  -drive format=raw,file=build/myos-current.img,if=floppy -boot order=a \
  -nic user,model=rtl8139
```

At `myos>`:

```text
net status
net up
net dns localhost
net get 10.0.2.2 8000 /
```

For the last command, start a host HTTP server on port 8000, for example
`python3 -m http.server 8000 --bind 127.0.0.1` in another WSL terminal.
QEMU's user network normally assigns 10.0.2.15 to the guest and exposes
the host at 10.0.2.2. `net udp IP PORT TEXT` sends a UDP datagram and waits
for a response. `net status` reports the DHCP address, gateway and DNS.
`net get HOST PORT PATH` resolves HOST, connects to TCP PORT, sends HTTP/1.0,
and prints the response. It does not support HTTPS; that is Phase 28.

The service layer exposes `net_http_fetch` with a response-chunk callback for
future in-kernel consumers. It is a deliberately serialized client: one
outstanding request, one TCP connection, 1,514-byte Ethernet frames and a
bounded 8 KiB receive ring. It validates packet lengths, IPv4/transport
checksums, destination and expected peer. Fragmented IPv4 packets, IPv6,
multiple sockets, TLS, DHCP lease renewal and a Ring 3 network syscall are
not yet supported. Browser integration and larger capacity remain separate
work in Phases 28–29.

## Validation

```bash
python3 tools/run_logged.py -- python3 tests/test_network.py
python3 tools/run_logged.py -- python3 tests/test_system.py
python3 tools/run_logged.py -- python3 tools/release.py build
python3 tools/run_logged.py -- python3 tests/test_network_bios.py
python3 tools/run_logged.py -- python3 tests/test_network_uefi.py
```

The network test boots MyOS with QEMU RTL8139 and its user network, obtains a
DHCP lease, resolves a name through QEMU DNS, exchanges UDP with a host echo
server, and retrieves HTTP from a host server. It then checks a refused TCP
connection, a timed-out UDP request, multi-packet HTTP streaming, successful
reconnection, a second DHCP request, and behavior without a supported NIC.
The packaged-image tests verify BIOS with a separate v2 data disk and UEFI
with RTL8139. The system suite regresses boot, memory, processes, GUI and
storage paths. Command transcripts are in `build/activity.log`;
the owner can watch with `tail -f build/activity.log` and pushes GitHub
commits manually.
