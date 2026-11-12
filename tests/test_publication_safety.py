import pathlib,re,unittest
ROOT=pathlib.Path(__file__).resolve().parents[1]
class PublicationSafety(unittest.TestCase):
 def test_no_channel_brand_in_firmware(self):
  for f in (ROOT/'firmware').rglob('*'):
   if f.suffix in ('.cpp','.h','.ini'):
    self.assertNotRegex(f.read_text(errors='replace'),r'(?i)massacreto|massa-creto|youtube\.com/@massacreto',str(f))
 def test_no_obvious_google_key(self):
  for folder in ('firmware','installer-web'):
   for f in (ROOT/folder).rglob('*'):
    if f.is_file() and f.suffix in ('.cpp','.h','.ini','.html','.js','.json'):
     self.assertNotRegex(f.read_text(errors='replace'),r'AIza[0-9A-Za-z_-]{35}',str(f))
 def test_no_generated_artifacts(self):
  ignore=(ROOT/'.gitignore').read_text()
  self.assertIn('__pycache__/',ignore)
  self.assertIn('*.bin',ignore)
if __name__=='__main__':unittest.main()
