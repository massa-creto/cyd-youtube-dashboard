import pathlib, re, unittest
ROOT=pathlib.Path(__file__).resolve().parents[1]
SOURCE=(ROOT/'firmware/src/main.cpp').read_text()
INI=(ROOT/'firmware/platformio.ini').read_text()
POLICY=(ROOT/'firmware/include/SecurityPolicy.h').read_text()
class ReleaseInvariants(unittest.TestCase):
    def test_only_st7789(self):
        self.assertIn('ST7789_DRIVER=1', INI)
        self.assertNotIn('ILI9341_DRIVER', INI)
    def test_tls_mode_is_explicit_and_documented(self):
        self.assertNotIn('setInsecure()', SOURCE)
        self.assertIn('client.setCACert(kGoogleTrustRoots)', SOURCE)
        self.assertIn('SecurityPolicy::clockReady()', SOURCE)
    def test_admin_protection(self):
        self.assertIn('adminHash', SOURCE)
        self.assertIn('passwordDigest(', SOURCE)
        self.assertIn('forcePasswordChange', SOURCE)
    def test_post_origin_policy(self):
        self.assertIn('SecurityPolicy::sameOrigin(origin, host)', SOURCE)
    def test_avatar_domain_restriction(self):
        self.assertIn('isGoogleImageHost(channelAvatarUrl)', SOURCE)
        self.assertIn("host.indexOf('@')", POLICY)
    def test_no_obvious_embedded_google_key(self):
        self.assertNotRegex(SOURCE, r'AIza[0-9A-Za-z_-]{30,}')
    def test_wifi_dependent_startup_poll(self):
        self.assertIn('firstPollPending', SOURCE)
        self.assertIn('WiFi.status() == WL_CONNECTED &&', SOURCE)
    def test_avatar_not_fetched_on_every_poll(self):
        self.assertIn('avatarRefreshPending', SOURCE)
        self.assertIn('void fetchAvatar(bool force = false)', SOURCE)
    def test_helpers_present(self):
        for n in ['SecurityPolicy.h','ColorUtils.h','GoogleTrustRoots.h']:
            self.assertTrue((ROOT/'firmware/include'/n).is_file())
if __name__=='__main__': unittest.main()
