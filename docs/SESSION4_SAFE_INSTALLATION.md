# Session 4 — Safe installation preparation

## Implemented
- Offline candidate generator `tools/prepare_install_candidate.py` reads Session 2 `build-report.json` and verifies SHA-256 and file sizes of real build artifacts.
- It emits **review-only** `cyd-install-candidate-v1` metadata, never an ESP Web Tools manifest, never URLs or executable flash instructions.
- GitHub Pages adds `/install-status/` with clear disabled installer status and outstanding acceptance gates.
- Existing `/diagnostics/` remains read-only. No firmware source, bootloader, partition table or device state was changed.

## Why install is blocked
No actual compiled bootloader/partition/application binaries were provided. Flash offsets, target flash size, boot mode, recovery and clean-device setup are not verified. This is a strict safety gate, not a UI omission. Do not guess offsets or create a flash manifest from assumptions.

## Offline review commands (only after obtaining actual CI build artifacts)
```sh
python tools/package_firmware.py --build-dir firmware/.pio/build/cyd_st7789 --out dist/review --version rc7-review --commit YOUR_REAL_COMMIT_SHA
python tools/prepare_install_candidate.py --report dist/review/build-report.json --out dist/review/install-candidate.review.json
python -m unittest discover -s tests -v
```

## Required for Session 5
- Download real firmware build artifacts from the matching CI commit; inspect partition map and build flash offsets.
- Read chip and flash size using a separately reviewed read-only tool, not USB VID/PID alone.
- Verify factory installation and full backup/recovery on a **spare** CYD before touching the working device.
- Validate first-run setup and credentials. Keep installer disabled until these tests pass.

## Security notes
`admin/admin` is intentionally the first-run bootstrap; the enforced password change remains unchanged. A full-flash backup is sensitive and must never be committed. The static install-status page has no network API, scripts or credentials.
