import pathlib,unittest
ROOT=pathlib.Path(__file__).resolve().parents[1]
class BrowserBackupTests(unittest.TestCase):
 def test_no_flash_commands(self):
  for file in ("backup-core.mjs",):
   s=(ROOT/"installer-web/backup"/file).read_text()
   for bad in ("erase_flash", "write_flash", "port.open(", "requestPort(", "port.write("):
    self.assertNotIn(bad,s)
 def test_preview_not_publicly_linked(self):
  page=(ROOT/"installer-web/index.html").read_text()
  self.assertNotIn('href="backup/"',page)
  self.assertIn('href="local-backup/"',page)
