import unittest
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
class PagesReleaseTests(unittest.TestCase):
 def test_no_unverified_flasher_button(self):
  s=(ROOT/'installer-web/index.html').read_text()
  self.assertIn('intentionally disabled',s)
  self.assertNotIn('esp-web-install-button',s)
 def test_pages_workflow_exists(self):
  self.assertIn('actions/deploy-pages@v4',(ROOT/'.github/workflows/pages.yml').read_text())
 def test_no_pretend_binaries(self):
  self.assertFalse(list((ROOT/'installer-web').glob('*.bin')))
