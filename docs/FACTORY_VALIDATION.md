# Factory installation validation

Factory flashing is **not available** in this release. A successful application build does not establish the correct bootloader offsets, flash capacity, partition layout, or recovery procedure.

## Evidence required

1. Download the firmware build artifact from the GitHub Actions run associated with the exact source commit. Record the 40-character SHA.
2. Inspect `bootloader.bin`, `partitions.bin`, and `firmware.bin` using `tools/verify_release_inputs.py`. This produces a review-only JSON report with SHA-256 hashes. It does **not** create a flash manifest.
3. Independently confirm bootloader and partition offsets from the actual PlatformIO/esptool build output. Do not assume a generic ESP32 layout.
4. Identify the actual ESP32 and flash capacity on the target hardware. USB-UART VID/PID alone is insufficient.
5. On a **spare** supported CYD, make a private flash backup, test a clean installation, verify display and touch, and demonstrate restoration from backup.
6. Record all results, including firmware provenance, test hardware, failure modes, and recovery outcome before enabling public installation.

## Offline artifact review

```bash
python tools/verify_release_inputs.py --build-dir path/to/extracted-build --commit FULL_40_CHARACTER_SHA --out local-review.json
python -m unittest discover -s tests -v
```

The report is not an ESP Web Tools manifest and cannot be used to install firmware. Keep private flash dumps out of the repository and CI artifacts.
