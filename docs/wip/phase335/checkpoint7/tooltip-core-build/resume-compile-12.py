from pathlib import Path
import os,subprocess
source=Path('/home/xytron404/myos/build/phase335-core-workspace/src')
env=os.environ.copy();env['PATH']=str(source.parent/'host-tools/prefix/usr/bin')+':'+str(source/'third_party/depot_tools')+':'+env['PATH'];env['DEPOT_TOOLS_UPDATE']='0';env['DEPOT_TOOLS_DIR']=str(source/'third_party/depot_tools')
print('Resuming identical source/output incrementally with12 workers; no source change.',flush=True)
subprocess.run([str(source/'third_party/ninja/ninja'),'-C','out/evergreen-x64_gold','-j12','cobalt'],cwd=source,env=env,check=True)
