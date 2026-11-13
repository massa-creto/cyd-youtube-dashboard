import hashlib,json,sys,tempfile,unittest
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/"tools"))
from prepare_install_candidate import prepare
ROOT=Path(__file__).resolve().parents[1]
class Session4Tests(unittest.TestCase):
 def make_report(self,dir):
  artifacts={}
  for name in ("firmware.bin","bootloader.bin","partitions.bin"):
   content=(name*30).encode();(dir/name).write_bytes(content)
   artifacts[name]={"bytes":len(content),"sha256":hashlib.sha256(content).hexdigest()}
  report={"status":"REVIEW_ONLY_NOT_FACTORY_VALIDATED","source_commit":"abcdef123456","chip_family":"ESP32","artifacts":artifacts,"flash_layout":{"initial_app":{"offset":65536},"nvs":{"offset":36864}}}
  p=dir/"build-report.json";p.write_text(json.dumps(report));return p
 def test_candidate_is_blocked(self):
  with tempfile.TemporaryDirectory() as temp:
   d=Path(temp);candidate=prepare(self.make_report(d),d/"candidate.review.json")
   self.assertFalse(candidate["install_enabled"]);self.assertIn("BLOCKED",candidate["status"])
   self.assertNotIn("builds",candidate)
 def test_reject_tampered_binary(self):
  with tempfile.TemporaryDirectory() as temp:
   d=Path(temp);report=self.make_report(d);(d/"firmware.bin").write_bytes(b"tampered")
   with self.assertRaises(ValueError):prepare(report,d/"candidate.review.json")
 def test_reject_unknown_commit(self):
  with tempfile.TemporaryDirectory() as temp:
   d=Path(temp);p=self.make_report(d);obj=json.loads(p.read_text());obj["source_commit"]="unknown";p.write_text(json.dumps(obj))
   with self.assertRaises(ValueError):prepare(p,d/"candidate.review.json")
 def test_no_installer_actions_in_site(self):
  page=(ROOT/"installer-web/install-status/index.html").read_text()
  self.assertIn("disabled",page);self.assertNotIn("<esp-web-install-button",page)
  self.assertNotIn("<script",page)
if __name__=="__main__":unittest.main()
