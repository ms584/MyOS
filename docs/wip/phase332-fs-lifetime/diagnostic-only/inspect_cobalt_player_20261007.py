"""Cache only required player/decode contracts from the audited source pin."""
import hashlib,json,urllib.request,urllib.error
from pathlib import Path
root=Path('/home/xytron404/myos')
pin=json.loads((root/'docs/PHASE33_COBALT_AUDIT.json').read_text())['source_commit']
out=root/'build/cobalt-player-audit-20261007';out.mkdir(exist_ok=True)
for name in ('starboard/player.h','starboard/decode_target.h','starboard/window.h',
             'ui/ozone/platform/starboard/ozone_platform_starboard.cc',
             'ui/ozone/platform/starboard/platform_window_starboard.cc',
             'starboard/extension/graphics.h','starboard/extension/gles.h',
             'starboard/android/shared/window_create.cc',
             'cobalt/browser/switches.cc','cobalt/browser/application.cc',
             'cobalt/browser/cobalt_browser_main_parts.cc',
             'cobalt/shell/browser/shell.cc','content/shell/browser/shell.cc',
             'cobalt/shell/common/shell_switches.cc','starboard/extension/font.h',
             'third_party/skia/src/ports/SkFontMgr_cobalt.cc',
             'cobalt/content/fonts/BUILD.gn','third_party/fonts/BUILD.gn',
             'cobalt/content/fonts/README.md'):
 target=out/name.replace('/','-')
 if not target.exists():
  try:
   with urllib.request.urlopen(f'https://raw.githubusercontent.com/youtube/cobalt/{pin}/{name}',timeout=25) as response:
    target.write_bytes(response.read())
  except urllib.error.HTTPError as error:
   print(name,'HTTP',error.code,flush=True);continue
 print(pin,name,len(target.read_bytes()),hashlib.sha256(target.read_bytes()).hexdigest(),target,flush=True)
