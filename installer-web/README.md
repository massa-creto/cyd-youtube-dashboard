# GitHub Pages installer landing (RC2)
This is a safe placeholder, not a working flasher. It will be deployed to GitHub Pages by `.github/workflows/pages.yml`. Full clean-install binaries and verified offsets are required before enabling browser flashing.


**Important:** `tools/make_web_release.py` is a review-only generator. It is not connected to Pages deployment. Its application-only manifest is not a factory installer, and its offsets must be independently verified on the target device before use.


The diagnostics section provides `/diagnostics/` for read-only USB metadata and simulation. It does not identify the ESP32 chip, read flash, or install firmware. HTTPS or localhost and Chrome/Edge are required for real Web Serial selection.

Recovery and unbrick instructions are published at `recovery/`. They do not perform writes. Browser backup and factory flashing remain disabled pending hardware validation.
