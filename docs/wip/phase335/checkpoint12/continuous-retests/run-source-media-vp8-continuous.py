from pathlib import Path
import os,sys,runpy
root=Path('/home/xytron404/myos')
private=root/'build/phase335-abort-worktree'
assert Path.cwd()==private
os.environ['MYOS_SOURCE_CORE_MANIFEST']=str(root/'build/phase335-core-workspace/core-build-identity.json')
os.environ['MYOS_RENDERER_PROVIDER_PATH']=str(private/'build/media-evaluation/phase335-source-vp8')
os.environ['MYOS_MEDIA_NATIVE_PCM_CAPTURE']='1'
os.environ['MYOS_MEDIA_RAM_DIAGNOSTIC']='1'
os.environ['MYOS_MEDIA_CONTINUOUS']='1'
os.environ['MYOS_MEDIA_KERNEL_ELF']=str(private/'build/kernel.elf')
os.environ.pop('MYOS_MEDIA_HEAP_DIAGNOSTIC',None)
sys.path.insert(0,str(private/'tests'))
sys.path.insert(0,str(private/'docs/wip/native-pmm512/kvm-ata-20261008'))
sys.argv=['tests/test_cobalt_media_observation.py','/home/xytron404/cd-QJYMwG/build/cobalt-resources-dwxalc2n','/home/xytron404/platform-fonts-01W7Ri/build/platform-fonts-20261007']
runpy.run_path('tests/test_cobalt_media_observation.py',run_name='__main__')
