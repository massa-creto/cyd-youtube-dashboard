import pathlib,unittest
R=pathlib.Path(__file__).resolve().parents[1]
class TestBuildFix(unittest.TestCase):
 def test_clock_guard_syntax(self):
  s=(R/'firmware/src/main.cpp').read_text()
  self.assertIn('if (!SecurityPolicy::clockReady())',s)
  self.assertNotIn('if !SecurityPolicy::clockReady()',s)
 def test_author_reference(self):
  self.assertIn('https://www.youtube.com/@MassaCreto',(R/'README.md').read_text())
  self.assertIn('https://www.youtube.com/@MassaCreto',(R/'BRANDING.md').read_text())
 def test_no_channel_brand_in_firmware(self):
  for p in (R/'firmware').rglob('*'):
   if p.is_file() and p.suffix in ('.cpp','.h','.ini'):
    self.assertNotIn('massacreto',p.read_text().lower())
