"""Exercise production Windows prefix diagnosis against ordinary file fixtures only."""
from pathlib import Path
import sys, json, hashlib, base64, shutil, subprocess
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tools'))
from generate_phase25_prefix_diagnostic import generate,IMAGE
SCRIPT=ROOT/'tools/diagnose_phase25_usb_prefix.ps1'
OUT=ROOT/'build/phase25-prefix-diagnostic-tests'
def sha(b):return hashlib.sha256(b).hexdigest()
def win(p):return subprocess.check_output(['wslpath','-w',str(p)],text=True).strip()
def run():
    assert SCRIPT.read_text()==generate(),'Generated script stale'
    OUT.mkdir(exist_ok=False)
    fixture=OUT/'ordinary diagnostic fixture.img';shutil.copyfile(IMAGE,fixture)
    original=IMAGE.read_bytes();results=[]
    def check(name,changed):
        before=fixture.read_bytes()
        p=subprocess.run(['powershell.exe','-NoProfile','-NonInteractive','-ExecutionPolicy','Bypass','-File',win(SCRIPT),'-FixturePath',win(fixture)],capture_output=True,timeout=90)
        (OUT/(name+'.stdout')).write_bytes(p.stdout);(OUT/(name+'.stderr')).write_bytes(p.stderr)
        assert p.returncode==0,(p.stdout[-2000:],p.stderr)
        d=json.loads(p.stdout.decode('utf-8-sig'))
        assert not d['physical'] and d['no_physical_write'] and not d['snapshot_area_read']
        assert d['result']==('MISMATCH' if changed else 'MATCH')
        assert d['chunks_checked']==544
        assert [c['offset'] for c in d['changed_chunks']]==[i*65536 for i in changed]
        assert d['captured_chunks']==min(16,len(changed))
        assert d['capture_truncated']==(len(changed)>16)
        for j,c in enumerate(d['changed_chunks']):
            o=c['offset'];actual=before[o:o+65536]
            assert c['actual_sha256']==sha(actual) and c['expected_sha256']==sha(original[o:o+65536])
            if j<16:assert base64.b64decode(c['content_base64'],validate=True)==actual
            else:assert c['content_base64'] is None
        assert fixture.read_bytes()==before
        results.append({'name':name,'changed_chunks':len(changed),'captured_chunks':d['captured_chunks'],'input_unchanged':True,'result':'PASS'})
    check('pristine',[])
    with fixture.open('r+b') as f:
        f.seek(35652096);f.write(b'preserved mutable snapshot, ignored by diagnostic')
    check('mutable-save',[])
    with fixture.open('r+b') as f:
        for i in [0,16,543]:
            o=i*65536+17;f.seek(o);v=f.read(1);f.seek(o);f.write(bytes([v[0]^1]))
    check('mbr-esp-final-prefix',[0,16,543])
    shutil.copyfile(IMAGE,fixture)
    with fixture.open('r+b') as f:
        for i in range(17):
            f.seek(i*65536+100);v=f.read(1);f.seek(i*65536+100);f.write(bytes([v[0]^1]))
    check('bounded-export',list(range(17)))
    # Guards and read-only native open are copied byte-for-byte from tested verifier.
    verifier=(ROOT/'tools/verify_phase25_usb_readback.ps1').read_text()
    assert verifier[verifier.index('function Assert-SelectedDisk'):verifier.index('# These digests')] in SCRIPT.read_text()
    assert verifier[verifier.index('$isFixture ='):verifier.index('# Both modes')] in SCRIPT.read_text()
    assert IMAGE.read_bytes()==original
    report={'script_sha256':sha(SCRIPT.read_bytes()),'generator_sha256':sha((ROOT/'tools/generate_phase25_prefix_diagnostic.py').read_bytes()),'tests':results,'generated_reproducible':True,'unchanged_production_guards_and_native_read_only_open':True,'physical_device_opened':False,'physical_diagnostic':'NOT RUN'}
    (OUT/'results.json').write_text(json.dumps(report,indent=2)+'\n')
    print('PASS: actual Windows PowerShell diagnostic; pristine/mutable-save/MBR+ESP+last-prefix changes/17chunk export bound; independent payload hashes; all inputs unchanged. File fixtures ONLY.')
if __name__=='__main__':run()