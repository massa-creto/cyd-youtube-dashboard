#!/usr/bin/env python3
"""Read-only ESP32 full-flash backup. No write/erase commands."""
import argparse, datetime, hashlib, json, re, subprocess, sys
from pathlib import Path

ALLOWED_SIZES_MB = (2, 4, 8, 16)

def parse_flash_size(output):
    """Accept esptool v4/v5 output; fail closed on ambiguous sizes."""
    matches = re.findall(r'(?:Detected\s+flash\s+size|Flash\s+size)\s*:\s*(\d+)\s*(MB|MiB|KB|KiB)\b', output, re.I)
    values = set()
    for amount, unit in matches:
        size = int(amount) * (1024 * 1024 if unit.lower() in ('mb', 'mib') else 1024)
        values.add(size)
    if len(values) != 1:
        raise ValueError('Flash size missing or ambiguous')
    size = values.pop()
    if size not in [n * 1024 * 1024 for n in ALLOWED_SIZES_MB]:
        raise ValueError('Flash size outside supported range')
    return size

def main():
    p = argparse.ArgumentParser(description='Read-only full flash backup; may contain secrets')
    p.add_argument('--port', required=True)
    p.add_argument('--out', required=True, type=Path)
    p.add_argument('--confirm-read', action='store_true')
    p.add_argument('--expected-size-mb', type=int, choices=ALLOWED_SIZES_MB,
                   help='Optional independent capacity cross-check (e.g. verified 4MB)')
    a = p.parse_args()
    if not re.fullmatch(r'COM\d{1,3}', a.port, re.I): p.error('Expected a Windows COM port')
    if not a.confirm_read: p.error('Explicit --confirm-read required')
    if a.out.exists() and any(a.out.iterdir()): p.error('Output directory must be empty')
    cmd = [sys.executable, '-m', 'esptool', '--chip', 'esp32', '--port', a.port]
    probe = subprocess.run(cmd + ['flash-id'], capture_output=True, text=True)
    if probe.returncode:
        sys.exit('Flash probe failed; no backup performed:\n' + (probe.stdout + probe.stderr)[-1200:])
    try: size = parse_flash_size(probe.stdout + '\n' + probe.stderr)
    except ValueError as exc:
        sys.exit(f'{exc}; no backup performed.\n' + probe.stdout[-1200:])
    if a.expected_size_mb and size != a.expected_size_mb * 1024 * 1024:
        sys.exit('Detected flash size differs from independently confirmed capacity; refusing backup')
    a.out.mkdir(parents=True, exist_ok=True)
    dest = a.out / 'private_flash_backup.bin'
    result = subprocess.run(cmd + ['--baud', '115200', 'read-flash', '0x0', hex(size), str(dest)])
    if result.returncode or not dest.is_file() or dest.stat().st_size != size:
        sys.exit('Read incomplete; do not rely on this backup')
    digest = hashlib.sha256(dest.read_bytes()).hexdigest()
    metadata = {'timestamp_utc': datetime.datetime.now(datetime.timezone.utc).isoformat(),
                'chip': 'esp32', 'size_bytes': size, 'sha256': digest, 'contains_secrets': True,
                'note': 'Read-back archive only; restore not tested'}
    (a.out / 'backup_metadata.json').write_text(json.dumps(metadata, indent=2) + '\n', encoding='utf-8')
    print('Backup complete. Keep private; never commit to GitHub. SHA256:', digest)

if __name__ == '__main__': main()
