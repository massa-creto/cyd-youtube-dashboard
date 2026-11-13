# Web Flasher dependency and readback gate

This checkpoint adds fail-closed chip/flash-capacity checks and exact readback-length validation. **No physical flash backup, flash write or erase is implemented.**

The browser uses an external `esptool-js@0.7.0` bundle URL. The bundle URL and global export shape **could not be verified from this offline environment**. The existing dynamic script loader must not be considered production-ready until a real browser validates the URL and the expected `window.esptooljs` exports. Prefer vendoring an independently verified, pinned build with its SHA-256 recorded, then testing on GitHub Pages. Do not publish an unverified dependency as a working installer.

## Acceptance checks before hardware read

1. Fetch pinned dependency in a connected build environment and inspect its actual browser exports.
2. Verify module/global compatibility, source version, license, and SHA-256; vendor the audited artifact.
3. Test Web Serial connect and chip identification with the user's device.
4. Determine flash capacity from trusted device reporting, not manual JEDEC input.
5. Implement read-only backup using the verified library API and compare actual bytes to capacity.
6. Save locally; warn about credentials; test independent restore on spare hardware before enabling writes.

**Safety:** The legacy custom ROM controls are experimental and must not be used as evidence of compatibility. All write/erase UI remains disabled.
