import pathlib, unittest
s=(pathlib.Path(__file__).resolve().parents[1]/'firmware/src/main.cpp').read_text()
class AvatarPolicy(unittest.TestCase):
    def test_refresh_flag(self):
        self.assertIn('bool avatarRefreshPending = true;',s)
        self.assertIn('if (avatarRefreshPending)',s)
    def test_timing_diagnostics(self):
        self.assertIn('const uint32_t requestStartMs = millis();',s)
    def test_verified_tls(self):
        self.assertNotIn('setInsecure()',s)
        self.assertIn('client.setCACert(kGoogleTrustRoots)',s)
if __name__=='__main__': unittest.main()
