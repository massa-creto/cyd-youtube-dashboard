# Session 2 — Firmware packaging and recovery preparation

## What is confirmed
- PlatformIO configuration targets `esp32dev` / classic ESP32 and `cyd_st7789`.
- Source archive includes no compiled firmware build output; a local PlatformIO compiler was not available for this session.
- `tools/flash_layout.py` parses actual partition binaries, validates overlap and NVS/app presence, and checks application size.
- `tools/package_firmware.py` creates a **review-only** SHA-256 inventory from actual build output. It deliberately does **not** emit a factory manifest or merged flash image.
- `tools/backup_flash_windows.py` is an opt-in, read-only backup helper. No device operations were performed.

## Run on a build-capable machine
```sh
pio run -d firmware -e cyd_st7789
python tools/flash_layout.py firmware/.pio/build/cyd_st7789
python tools/package_firmware.py --build-dir firmware/.pio/build/cyd_st7789 --out dist/review --version rc7-review --commit YOUR_COMMIT_SHA
python -m unittest discover -s tests -v
```

## Why no merged factory image yet
A valid factory image requires verified bootloader offset, partition table offset, boot_app0 / OTA initialization requirements, flash mode/size, and real device flash characteristics. Neither the current source ZIP nor the previous CI screenshot proves all of these. Creating a merged image with guessed offsets would be inappropriate for the working CYD.

## Recovery safety
A full readback backup can contain plaintext or recoverable private configuration. Do not publish it. A valid backup is not proof that restore is safe; test recovery on a spare CYD before using a production device.

## Session 3 handoff
Build a browser **read-only** detection UI, with mock tests. Do not expose an install button until a factory image and hardware recovery are validated. Use the exact Session 2 checkpoint ZIP.
