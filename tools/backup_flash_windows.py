#!/usr/bin/env python3
"""Explicit opt-in, local read-only full-flash backup using esptool. Never writes device."""
import argparse, datetime, hashlib, json, re, subprocess, sys
from pathlib import Path

def main():
    p=argparse.ArgumentParser(description='Read-only ESP32 flash backup. Contains private credentials: keep offline.')
    p.add_argument('--port',required=True,help='Windows serial port, e.g. COM5')
    p.add_argument('--out',required=True,type=Path,help='Private output directory outside repository')
    p.add_argument('--confirm-read',action='store_true',help='Explicitly allow reading the chip')
    a=p.parse_args()
    if not re.fullmatch(r'COM\d{1,3}',a.port,re.I):p.error('Only Windows COM ports are accepted')
    if not a.confirm_read:p.error('No device access without --confirm-read')
    if a.out.exists() and any(a.out.iterdir()):p.error('Output directory must be empty')
    cmd=[sys.executable,'-m','esptool','--chip','esp32','--port',a.port]
    probe=subprocess.run(cmd+['flash_id'],capture_output=True,text=True)
    if probe.returncode:sys.exit('Flash probe failed; no backup performed:\n'+probe.stderr[-1500:])
    match=re.search(r'Detected flash size:\s*(\d+)\s*MB',probe.stdout+'\n'+probe.stderr,re.I)
    if not match:sys.exit('Could not confidently determine flash size; refusing to read.\n'+probe.stdout[-1200:])
    size=int(match.group(1))*1024*1024
    if size not in (2,4,8,16):sys.exit('Unexpected flash size; manual review required')
    a.out.mkdir(parents=True,exist_ok=True)
    dest=a.out/'private_flash_backup.bin'
    result=subprocess.run(cmd+['read_flash','0x0',hex(size),str(dest)])
    if result.returncode or not dest.exists() or dest.stat().st_size!=size:sys.exit('Read incomplete; do not rely on this backup')
    digest=hashlib.sha256(dest.read_bytes()).hexdigest()
    (a.out/'backup_metadata.json').write_text(json.dumps({'timestamp_utc':datetime.datetime.now(datetime.timezone.utc).isoformat(),'chip':'esp32','port':a.port,'size_bytes':size,'sha256':digest,'contains_secrets':True},indent=2)+'\n')
    print('Backup complete. Keep private; never commit to GitHub. SHA256:',digest)
if __name__=='__main__':main()
