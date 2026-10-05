import importlib.util
import json
from pathlib import Path
import shutil
import tempfile
import unittest

SOURCE = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location('profiles', SOURCE/'generate_profiles.py')
profiles = importlib.util.module_from_spec(spec)
spec.loader.exec_module(profiles)

class Profiles(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.addCleanup(self.tmp.cleanup)
        self.root = Path(self.tmp.name)/'compatibility'
        shutil.copytree(SOURCE.parents[1]/'compatibility', self.root)
        # Synthetic optional write exercises the generic catalog after the
        # texture correction moved into mandatory runtime code.
        (self.root/'patches').mkdir(exist_ok=True)
        item={'schema':1,'id':'fixture-write','symbol':'NP_FIX_FIXTURE','bit':8,
              'runtime_sha256':profiles.RUNTIME,'environments':['pcsx2'],
              'writes':[{'address':'0x00210000','expected':'01000f24','replacement':'00000f24'}],
              'validation':{'status':'tested-experimental','tested_environments':['pcsx2'],
                            'evidence':'synthetic','notes':'Synthetic generator regression'}}
        (self.root/'patches/fixture-write.json').write_text(json.dumps(item))

    def change(self, relative, edit):
        path = self.root/relative
        value = json.loads(path.read_text())
        edit(value)
        path.write_text(json.dumps(value))

    def rejected(self):
        with self.assertRaises(ValueError): profiles.render(self.root)

    def test_generated_catalog_is_current(self):
        repo = SOURCE.parents[1]
        for name, content in profiles.render(repo/'compatibility').items():
            self.assertEqual((repo/name).read_text(), content)

    def test_unknown_fix(self):
        self.change('profiles/doom-us.json', lambda p: p['fixes'].append('untested'))
        self.rejected()

    def test_inexact_hash(self):
        self.change('profiles/doom-us.json', lambda p: p.update(disc_sha256='00'*31))
        self.rejected()

    def test_untested_environment(self):
        self.change('profiles/doom-us.json', lambda p: p['environments'].append('ps2'))
        self.rejected()

    def test_runtime_revision(self):
        self.change('patches/fixture-write.json', lambda p: p.update(runtime_sha256='00'*32))
        self.rejected()

    def test_ambiguous_identity(self):
        p = json.loads((self.root/'profiles/doom-us.json').read_text())
        p['id'] = 'duplicate'
        (self.root/'profiles/duplicate.json').write_text(json.dumps(p))
        self.rejected()

    def test_overlapping_writes(self):
        self.change('patches/fixture-write.json', lambda p: p['writes'].append(dict(p['writes'][0])))
        self.rejected()

    def test_expected_bytes_length(self):
        self.change('patches/fixture-write.json', lambda p: p['writes'][0].update(expected='00'))
        self.rejected()

    def test_duplicate_fix_bit(self):
        self.change('patches/fixture-write.json', lambda p: p.update(bit=2))
        self.rejected()

    def test_conflicting_profiles(self):
        p = json.loads((self.root/'patches/fixture-write.json').read_text())
        p.update(id='other', bit=16, symbol='NP_FIX_OTHER')
        (self.root/'patches/other.json').write_text(json.dumps(p))
        self.change('profiles/doom-us.json', lambda p: p['fixes'].extend(['fixture-write','other']))
        self.rejected()

    def test_retired_core_bit(self):
        self.change('patches/fixture-write.json', lambda p: p.update(bit=4))
        self.rejected()

    def test_reserved_core_address(self):
        self.change('patches/fixture-write.json', lambda p: p['writes'][0].update(address='0x0021d6a4'))
        self.rejected()

if __name__ == '__main__': unittest.main()
