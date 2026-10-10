"""Reject altered provenance or acceptance claims for the actual source-built core."""
from pathlib import Path
import json,sys,tempfile,unittest
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tools'))
from cobalt_source_core import load_source_core
WORKSPACE=ROOT/'build/phase335-core-workspace'
MANIFEST=WORKSPACE/'core-build-identity.json'

class CoreIdentityTest(unittest.TestCase):
    def test_actual_built_core(self):
        core,data=load_source_core(MANIFEST)
        self.assertEqual(core.stat().st_size,data['core_bytes'])
        self.assertFalse(data['official_google_binary'])
        self.assertFalse(data['phase_passed'])

    def test_tampering_and_false_acceptance_rejected(self):
        original=json.loads(MANIFEST.read_text())
        changes={'build_completed':False,'official_google_binary':True,
                 'source_commit':'0'*40,'core_bytes':1,'core_sha256':'0'*64,
                 'source_patch_sha256':'0'*64,'phase_passed':True}
        with tempfile.TemporaryDirectory(prefix='identity-test-',dir=WORKSPACE) as directory:
            candidate=Path(directory)/'altered.json'
            for key,value in changes.items():
                with self.subTest(field=key):
                    data=dict(original);data[key]=value
                    candidate.write_text(json.dumps(data))
                    with self.assertRaises(AssertionError):
                        load_source_core(candidate)

    def test_manifest_outside_workspace_rejected(self):
        with self.assertRaises(AssertionError):
            load_source_core(ROOT/'docs/PHASE33_COBALT_AUDIT.json')

if __name__=='__main__':
    unittest.main()
