# Official esptool-js integration checkpoint

The backup page adds a separate experimental device-identification path using Espressif's esptool-js v0.7.0. It does not expose `writeFlash`, `eraseFlash`, or `readFlash` operations. The legacy custom-ROM diagnostics remain visible for compatibility but are deprecated and should be removed after successful hardware verification of the official adapter.

## Important limitations

- The CDN bundle global export and actual Web Serial connection **have not been validated in a real browser**. A production release should vendor and pin a verified bundle and its checksum rather than depend on a CDN at runtime.
- `ESPLoader.main()` can reset the device and load a helper into RAM. It does not modify the flash in this implementation.
- The firmware only targets the original ESP32 CYD hardware profile. A chip name alone does not prove the display, touch controller, flash capacity, or pinout match.
- Full flash backup is feasible via the documented `ESPLoader.readFlash` API, but is not yet enabled; first determine flash size from the device, verify the backup length/hash, and test with the physical board.
- Browser flashing remains **disabled** until exact binaries/offsets, backup, recovery and first-boot behavior have been verified.
- Full backup files may contain passwords, tokens and personal data. Never upload them to a public repository.

## Validation

Run `node --test installer-web/backup/*.test.mjs` and `python -m pytest -q tests`. No hardware or browser testing is implied by these tests.
