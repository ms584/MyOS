"""Bounded, secret-free summary of the specific native HTTPS observation."""
from pathlib import Path
from collections import Counter
import json
import sys
from urllib.parse import urlsplit, parse_qs

root = Path('/home/xytron404/cobalt-closed-imports-xnVZeH')
bundle = root / 'build' / sys.argv[1]
journal = bundle / 'request-events.jsonl'
events = [json.loads(line) for line in journal.read_text().splitlines()] if journal.exists() else []
counts = Counter()
for event in events:
    url = urlsplit(event['path'])
    query = parse_qs(url.query)
    counts[query.get('kind', [url.path])[0]] += 1
print('BUNDLE', bundle.name, 'REQUESTS', len(events), 'KINDS', dict(counts))
for event in events[-8:]:
    print('EVENT', event)
serial = bundle / 'serial.txt'
if not serial.exists():
    serial = root / 'build/system-tests/native-verified-tls.log'
if serial.exists():
    lines = serial.read_text(errors='replace').splitlines()
    selected = [line for line in lines if any(word in line for word in (
        'Navigation throttle', 'DidFinishNavigation', 'net::ERR_',
        'handshake failed', 'PANIC', 'heap', 'reclaim', 'pages', 'CSP'))]
    print('SERIAL DIAGNOSTICS')
    print('\n'.join(selected[-18:]))
    print('SERIAL TAIL')
    print('\n'.join(lines[-5:]))
evidence = bundle / 'evidence.json'
if evidence.exists():
    data = json.loads(evidence.read_text())
    print('PEAK CONCURRENT', data['peak_concurrent_assets'])
    print('VISUAL', data['visual'])
    print('INPUTS', data['inputs'])
