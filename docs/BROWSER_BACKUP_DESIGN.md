# Browser flash backup — design and validation status

## Scope
A local-only backup experience for new and previously flashed ESP32 CYD boards. The UI now includes privacy and recovery warnings, consent gates and an offline simulation. **It does not currently read a physical board.**

`installer-web/backup/backup-core.mjs` implements a transport-agnostic, read-only streaming workflow. Its adapter contract requires `identify()` and `readFlash(offset,length)` and rejects unknown chip identity, unverified flash sizes, incomplete chunks and absent adapters. It computes SHA-256 using the browser's Web Crypto API and marks generated metadata as not verified by an independent device readback.

## Why physical backup is disabled
A real browser implementation needs a tested ESP32 ROM serial transport (e.g., a vetted compatible implementation of the esptool protocol), robust flash capacity detection, progress/cancellation, readback verification, and safe local download semantics. Web Serial availability alone is not proof of any of these. A simulation does not replace hardware validation. No network service, write operation or erase capability is included in the new page.

## Next engineering milestone
1. Select and audit a pinned, license-compatible ESP32 ROM serial transport implementation.
2. Add adapter integration behind an explicit capability gate and test on a spare board.
3. Save full flash to a user-selected local file, compute SHA-256 and verify saved length/hash.
4. Validate independent readback or second read where feasible, error recovery and disconnect handling.
5. Block all installation paths until backup succeeds and file persistence is confirmed; only then review any flash-writing UI.

## Privacy and recovery
Flash images may contain Wi-Fi credentials, tokens, keys and device-specific calibration. Never upload them to CI, GitHub, or support tickets. Flash encryption, secure boot or damaged flash can limit recovery. Refer to `docs/WEB_FLASHER_SAFETY_AND_RECOVERY.md`.
