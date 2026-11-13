import pathlib, unittest
ROOT=pathlib.Path(__file__).resolve().parents[1]
class TestReadOnlyDiagnostics(unittest.TestCase):
 def test_no_serial_write_or_open(self):
  js=(ROOT/'installer-web/diagnostics/app.js').read_text()
  for prohibited in ('port.open(','.write(','.setSignals(','.forget(','.close(', 'erase_flash', 'esp_loader', 'installFirmware'):
   self.assertNotIn(prohibited,js)
  self.assertIn('navigator.serial.requestPort()',js)
  self.assertIn('port.getInfo()',js)
 def test_install_disabled(self):
  js=(ROOT/'installer-web/diagnostics/app.js').read_text()
  self.assertIn('no flashing code is included',js)
 def test_page_links_and_csp(self):
  page=(ROOT/'installer-web/diagnostics/index.html').read_text()
  home=(ROOT/'installer-web/index.html').read_text()
  self.assertIn("connect-src 'none'",page)
  self.assertIn('diagnostics/',home)
  self.assertNotIn('Install now',page)
if __name__=='__main__':unittest.main()
