import pathlib
import unittest

SOURCE = (pathlib.Path(__file__).resolve().parents[1] / "firmware/src/main.cpp").read_text()

class UptimeSidebarTests(unittest.TestCase):
    def test_footer_label(self):
        self.assertIn("id='device-uptime'", SOURCE)
        self.assertIn(".sidebar-uptime{", SOURCE)
    def test_endpoint_auth(self):
        self.assertIn('web.on("/api/system/uptime", HTTP_GET, []() {\n    if (!requireAdmin()) return;', SOURCE)
    def test_64bit_clock(self):
        self.assertIn('esp_timer_get_time()', SOURCE)
    def test_browser_refresh(self):
        self.assertIn('setInterval(render,1000)', SOURCE)
        self.assertIn('setInterval(sync,30000)', SOURCE)

if __name__ == "__main__": unittest.main()
