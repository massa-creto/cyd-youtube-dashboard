import pathlib
import unittest

ROOT = pathlib.Path(__file__).resolve().parents[1]
SOURCE = (ROOT / "firmware/src/main.cpp").read_text()

class DiagnosticsUiTests(unittest.TestCase):
    def test_bounded_ring(self):
        self.assertIn("kDiagnosticCapacity = 12", SOURCE)
    def test_endpoints_require_admin(self):
        for method in ('HTTP_GET', 'HTTP_POST'):
            self.assertIn('web.on("/api/diagnostics", '+method+', []() {\n    if (!requireAdmin()) return;', SOURCE)
    def test_compact_appearance(self):
        self.assertIn(".appearance-layout{display:grid", SOURCE)
    def test_debug_opt_in(self):
        self.assertIn('bool debugEnabled = false;', SOURCE)

if __name__ == "__main__": unittest.main()
