#!/usr/bin/env python3
"""Validate a local CI artifact bundle without flashing, merging, or publishing it."""
import argparse, hashlib, json, re, sys
from pathlib import Path
from flash_layout import inspect

def validate(build_dir, commit):
    if not re.fullmatch(r"[0-9a-fA-F]{40}", commit):
        raise ValueError("Provide the exact 40-character Git commit SHA")
    build_dir = Path(build_dir)
    info = inspect(build_dir)
    required = ("bootloader.bin", "partitions.bin", "firmware.bin")
    artifacts = {}
    for name in required:
        p = build_dir / name
        if not p.is_file() or p.stat().st_size < (32 if name == "partitions.bin" else 256):
            raise ValueError("Missing or implausible build artifact: " + name)
        artifacts[name] = {"size": p.stat().st_size, "sha256": hashlib.sha256(p.read_bytes()).hexdigest()}
    return {"status": "REVIEW_ONLY", "source_commit": commit.lower(), "partition_layout": info,
            "artifacts": artifacts, "factory_flash_authorized": False,
            "hardware_checks": ["ESP32 chip and actual flash size", "bootloader/partition offsets from build", "spare board clean boot", "recovery on spare board"]}

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument("--build-dir", type=Path, required=True)
    p.add_argument("--commit", required=True)
    p.add_argument("--out", type=Path, required=True)
    args=p.parse_args()
    result=validate(args.build_dir,args.commit)
    args.out.parent.mkdir(parents=True,exist_ok=True)
    args.out.write_text(json.dumps(result,indent=2)+"\n",encoding="utf-8")
    print("Build artifacts reviewed; hardware approval NOT granted.")
if __name__ == "__main__":
    try: main()
    except (OSError,ValueError) as exc: sys.exit(str(exc))
