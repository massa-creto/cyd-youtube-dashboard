import sys, unittest, struct
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'tools'))
from flash_layout import parse_partitions

def ent(typ,sub,offset,size,name):
    return struct.pack('<HBBII16sI',0x50aa,typ,sub,offset,size,name.encode().ljust(16,b'\0'),0)
class TestLayout(unittest.TestCase):
    def test_normal(self):
        a=parse_partitions(ent(1,2,0x9000,0x5000,'nvs')+ent(0,0,0x10000,0x140000,'factory')+b'\xff'*32)
        self.assertEqual(a[1]['offset'],0x10000)
    def test_overlap(self):
        with self.assertRaises(ValueError):parse_partitions(ent(1,2,0x9000,0x9000,'nvs')+ent(0,0,0x10000,0x140000,'factory'))
    def test_invalid(self):
        with self.assertRaises(ValueError):parse_partitions(b'\x00'*32)
if __name__=='__main__':unittest.main()
