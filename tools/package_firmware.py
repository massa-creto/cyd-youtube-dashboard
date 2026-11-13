#!/usr/bin/env python3
"""Package verified build artifacts for review; no device I/O, no factory flash claims."""
import argparse, hashlib, json, shutil, subprocess, sys
from pathlib import Path
from flash_layout import inspect

def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def main():
    ap=argparse.ArgumentParser();ap.add_argument('--build-dir',required=True,type=Path);ap.add_argument('--out',required=True,type=Path);ap.add_argument('--version',default='rc7-review');ap.add_argument('--commit',default='unknown');a=ap.parse_args()
    b=a.build_dir; layout=inspect(b)
    required=['firmware.bin','bootloader.bin','partitions.bin']
    for name in required:
        if not (b/name).is_file() or (b/name).stat().st_size<256:raise ValueError('Missing/implausible '+name)
    if a.out.resolve()==b.resolve():raise ValueError('Output must differ from build directory')
    a.out.mkdir(parents=True,exist_ok=True)
    for name in required:shutil.copy2(b/name,a.out/name)
    optional=['boot_app0.bin']
    for name in optional:
        if (b/name).is_file():shutil.copy2(b/name,a.out/name)
    files={p.name:{'sha256':sha(p),'bytes':p.stat().st_size} for p in sorted(a.out.glob('*.bin'))}
    report={'version':a.version,'source_commit':a.commit,'chip_family':'ESP32','status':'REVIEW_ONLY_NOT_FACTORY_VALIDATED','flash_layout':layout,'artifacts':files,'warning':'No factory flash offsets are approved; do not install on hardware based on this report.'}
    (a.out/'build-report.json').write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8')
    (a.out/'SHA256SUMS.txt').write_text(''.join(f"{v['sha256']}  {k}\n" for k,v in files.items()),encoding='utf-8')
    print('Validated app partition size and created review-only artifacts; NO factory manifest or merged image created.')
if __name__=='__main__':
    try:main()
    except (ValueError,OSError) as e:sys.exit(str(e))
