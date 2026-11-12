import pathlib,unittest
ROOT=pathlib.Path(__file__).resolve().parents[1]
S=(ROOT/'firmware/src/main.cpp').read_text()
class SnapshotDefaults(unittest.TestCase):
 def test_reference_board_default(self):
  self.assertIn('snapshotColorMode = prefs.getUChar("snapmode", 1)',S)
  self.assertIn('uint8_t snapshotColorMode = 1;',S)
 def test_all_modes_remain(self):
  self.assertIn('Swap bytes (reference CYD default)',S)
  self.assertIn('Swap bytes + red / blue',S)
  self.assertIn('snapshotColorMode & 1',S)
  self.assertIn('snapshotColorMode & 2',S)
 def test_persistence_and_inversion(self):
  self.assertIn('prefs.putUChar("snapmode", snapshotColorMode)',S)
  self.assertIn('prefs.putBool("snapinvert", snapshotInverted)',S)
 def test_remote_touch_kept(self):
  self.assertIn('web.on("/api/display/touch", HTTP_POST',S)
  self.assertIn('processScreenTouch(sx.toInt(),sy.toInt())',S)
if __name__=='__main__':unittest.main()
