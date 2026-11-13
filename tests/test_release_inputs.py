import sys,unittest,tempfile,struct
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/"tools"))
from verify_release_inputs import validate

def entry(typ,sub,off,size,name):
    return struct.pack("<HBBII16sI",0x50aa,typ,sub,off,size,name.encode().ljust(16,b"\0"),0)
class ReleaseInputsTests(unittest.TestCase):
    def test_rejects_unverified_commit(self):
        with tempfile.TemporaryDirectory() as t:
            with self.assertRaises(ValueError): validate(t,"unknown")
    def test_rejects_missing_artifacts(self):
        with tempfile.TemporaryDirectory() as t:
            with self.assertRaises(OSError): validate(t,"a"*40)
    def test_valid_review_never_approves_factory(self):
        with tempfile.TemporaryDirectory() as t:
            p=Path(t)
            (p/"partitions.bin").write_bytes(entry(1,2,0x9000,0x5000,"nvs")+entry(0,0,0x10000,0x140000,"factory")+b"\xff"*32)
            (p/"bootloader.bin").write_bytes(b"X"*512)
            (p/"firmware.bin").write_bytes(b"Y"*1024)
            report=validate(p,"b"*40)
            self.assertFalse(report["factory_flash_authorized"])
            self.assertEqual(report["status"],"REVIEW_ONLY")
    def test_firmware_too_large(self):
        with tempfile.TemporaryDirectory() as t:
            p=Path(t)
            (p/"partitions.bin").write_bytes(entry(1,2,0x9000,0x5000,"nvs")+entry(0,0,0x10000,0x1000,"factory")+b"\xff"*32)
            (p/"firmware.bin").write_bytes(b"X"*5000)
            with self.assertRaises(ValueError):validate(p,"b"*40)
