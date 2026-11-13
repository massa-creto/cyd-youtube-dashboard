import pathlib,re,unittest
R=pathlib.Path(__file__).resolve().parents[1]
class RC3(unittest.TestCase):
 def test_no_channel_brand_in_firmware(self):
  for f in (R/'firmware').rglob('*'):
   if f.suffix in ('.cpp','.h','.ini'):
    self.assertNotRegex(f.read_text(),r'(?i)massacreto|massa.?creto',str(f))
 def test_no_factory_web_flasher(self):
  self.assertNotIn('esp-web-install-button',(R/'installer-web/index.html').read_text())
 def test_no_insecure_tls(self):
  self.assertNotIn('setInsecure(', (R/'firmware/src/main.cpp').read_text())
 def test_license_and_author(self):
  self.assertIn('Required Notice: Copyright 2026 MassaCreto',(R/'LICENSE').read_text())
  self.assertIn('CC BY-NC-SA 4.0',(R/'docs/LICENSE').read_text())
 def test_doc_assets(self):
  self.assertTrue((R/'docs/assets/architecture.svg').exists())
  self.assertTrue((R/'docs/PUBLIC_RELEASE_STATUS.md').exists())
if __name__=='__main__':unittest.main()
