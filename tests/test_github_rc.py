import pathlib,unittest
R=pathlib.Path(__file__).resolve().parents[1]
class GithubRC(unittest.TestCase):
 def test_tls_verified(self):
  s=(R/'firmware/src/main.cpp').read_text(); self.assertNotIn('setInsecure(',s); self.assertIn('client.setCACert(kGoogleTrustRoots)',s); self.assertIn('SecurityPolicy::clockReady()',s)
 def test_brand_defaults(self):
  s=(R/'firmware/src/main.cpp').read_text(); self.assertIn('CYD-YOUTUBE',s); self.assertNotIn('MASSACRETO-CYD',s)
 def test_licenses(self):
  self.assertIn('PolyForm Noncommercial', (R/'LICENSE').read_text()); self.assertIn('CC BY-NC-SA 4.0',(R/'docs/LICENSE').read_text())
 def test_no_embedded_binary(self):
  self.assertEqual([f.relative_to(R).as_posix() for f in R.rglob('firmware.bin')], ['installer-web/release/firmware.bin'])
