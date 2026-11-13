#!/usr/bin/env python3
"""Offline review manifest generator. Never produces a deployable ESP Web Tools manifest."""
import argparse,hashlib,json,sys
from pathlib import Path

def prepare(report_path,output):
    report=json.loads(Path(report_path).read_text(encoding="utf-8"))
    if report.get("status") != "REVIEW_ONLY_NOT_FACTORY_VALIDATED":
        raise ValueError("Unsupported build report status")
    if report.get("chip_family") != "ESP32":
        raise ValueError("Unsupported chip family")
    if not report.get("source_commit") or report["source_commit"] == "unknown":
        raise ValueError("Source commit must be identified")
    base=Path(report_path).resolve().parent
    required={"firmware.bin","bootloader.bin","partitions.bin"}
    artifacts=report.get("artifacts",{})
    if not required.issubset(artifacts):raise ValueError("Missing required binary metadata")
    checked={}
    for name,meta in artifacts.items():
        if Path(name).name!=name or not name.endswith(".bin"):
            raise ValueError("Unsafe artifact name")
        path=base/name
        if not path.is_file():raise ValueError("Missing artifact: "+name)
        size=path.stat().st_size
        digest=hashlib.sha256(path.read_bytes()).hexdigest()
        if size!=meta.get("bytes") or digest!=meta.get("sha256"):
            raise ValueError("Artifact integrity mismatch: "+name)
        checked[name]={"bytes":size,"sha256":digest}
    layout=report.get("flash_layout",{})
    if not layout.get("initial_app") or not layout.get("nvs"):
        raise ValueError("Incomplete partition layout")
    output=Path(output)
    output.parent.mkdir(parents=True,exist_ok=True)
    candidate={"schema":"cyd-install-candidate-v1","status":"BLOCKED_PENDING_HARDWARE_VALIDATION","install_enabled":False,"source_commit":report["source_commit"],"chip_family":"ESP32","artifacts":checked,"partition_summary":layout,"missing_approvals":["bootloader/partition/app offsets from real build","target flash size and board revision","factory boot on spare device","recovery restore on spare device","first-run onboarding","explicit maintainer release approval"],"note":"Review-only metadata. NOT an ESP Web Tools manifest. Never upload this as manifest.json."}
    output.write_text(json.dumps(candidate,indent=2)+"\n",encoding="utf-8")
    return candidate

if __name__=="__main__":
    p=argparse.ArgumentParser();p.add_argument("--report",required=True);p.add_argument("--out",required=True);a=p.parse_args()
    try:prepare(a.report,a.out);print("Review-only candidate generated. Installation remains disabled.")
    except (ValueError,OSError,KeyError) as exc:sys.exit(str(exc))
