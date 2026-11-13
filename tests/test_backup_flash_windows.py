import importlib.util
from pathlib import Path
import pytest
spec = importlib.util.spec_from_file_location('backup_flash_windows', Path(__file__).resolve().parents[1] / 'tools' / 'backup_flash_windows.py')
mod = importlib.util.module_from_spec(spec)
spec.loader.exec_module(mod)

def test_esptool_v5_4mb():
    assert mod.parse_flash_size('Flash Memory Information:\nDetected flash size: 4MB\n') == 4194304

def test_esptool_v4_4mb():
    assert mod.parse_flash_size('Detected flash size: 4MB') == 4194304

def test_ambiguous_fails():
    with pytest.raises(ValueError): mod.parse_flash_size('Detected flash size: 4MB\nFlash size: 8MB')

def test_missing_fails():
    with pytest.raises(ValueError): mod.parse_flash_size('Stub flasher running.')
