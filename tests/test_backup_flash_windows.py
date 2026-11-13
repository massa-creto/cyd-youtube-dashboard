import importlib.util
import unittest
from pathlib import Path

spec = importlib.util.spec_from_file_location(
    'backup_flash_windows', Path(__file__).resolve().parents[1] / 'tools' / 'backup_flash_windows.py'
)
mod = importlib.util.module_from_spec(spec)
spec.loader.exec_module(mod)


class BackupFlashWindowsTests(unittest.TestCase):
    def test_esptool_v5_4mb(self):
        self.assertEqual(mod.parse_flash_size('Flash Memory Information:\nDetected flash size: 4MB\n'), 4194304)

    def test_esptool_v4_4mb(self):
        self.assertEqual(mod.parse_flash_size('Detected flash size: 4MB'), 4194304)

    def test_ambiguous_fails(self):
        with self.assertRaises(ValueError):
            mod.parse_flash_size('Detected flash size: 4MB\nFlash size: 8MB')

    def test_missing_fails(self):
        with self.assertRaises(ValueError):
            mod.parse_flash_size('Stub flasher running.')


if __name__ == '__main__':
    unittest.main()
