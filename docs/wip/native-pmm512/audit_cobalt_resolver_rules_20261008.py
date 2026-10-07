"""Fetch only pinned primary-source routing code; never execute fetched content."""
from pathlib import Path
import urllib.request,hashlib
revision='3befd332486952f6603ae3baf2a8713878d6e200'
out=Path('build/cobalt-resolver-rules-audit-20261008');out.mkdir(exist_ok=True)
for name in ('services/network/network_context.cc','content/browser/network_service_instance_impl.cc','cobalt/shell/browser/shell_content_browser_client.cc'):
    url='https://raw.githubusercontent.com/youtube/cobalt/'+revision+'/'+name
    try:
        with urllib.request.urlopen(url,timeout=30) as response:data=response.read()
    except Exception as error:
        print(name,type(error).__name__,str(error));continue
    target=out/name.replace('/','-');target.write_bytes(data)
    print(name,len(data),hashlib.sha256(data).hexdigest())
    lines=data.decode().splitlines()
    for index,line in enumerate(lines):
        if 'HostResolverRules' in line or 'host_resolver_rules' in line:
            print('\n'.join(f'{n+1}: {lines[n]}' for n in range(max(0,index-3),min(len(lines),index+5))))
