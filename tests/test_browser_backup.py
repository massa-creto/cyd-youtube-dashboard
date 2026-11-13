import pathlib,unittest
ROOT=pathlib.Path(__file__).resolve().parents[1]
class BrowserBackupTests(unittest.TestCase):
 def test_no_flash_commands(self):
  for file in ("app.mjs","backup-core.mjs"):
   s=(ROOT/"installer-web/backup"/file).read_text()
   for bad in ("erase_flash", "write_flash", "port.open(", "requestPort(", "port.write("):
    self.assertNotIn(bad,s)
 def test_install_disabled(self):
  page=(ROOT/"installer-web/backup/index.html").read_text()
  self.assertIn("Back up connected device — not yet available",page)
  self.assertIn("disabled",page)
 def test_home_link(self):
  self.assertIn('href="backup/"',(ROOT/"installer-web/index.html").read_text())
