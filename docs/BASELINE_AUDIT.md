# Session 1 — Baseline audit

**Input:** user-supplied `CYD_YOUTUBE_DASHBOARD_RC7_GITHUB_BUILD_FIX(1).zip`. This is a source archive, not proof that its bytes match the latest remote Git commit. **Scope:** static source review and local Python tests. **No firmware modifications or flashing.**

## Verified
- 42/42 Python unittest checks pass in the analysis environment.
- `firmware/platformio.ini` defines only `cyd_st7789`, uses `esp32dev`, and pins `espressif32@6.12.0`.
- CI builds with PlatformIO and uploads application, bootloader, and partition binaries as review-only artifacts.
- Pages deploys a static landing page; public USB installation is not enabled.
- TLS client configuration loads bundled Google trust roots; no `setInsecure()` was found in the inspected source.
- Admin password hashes are stored with a salt; default `admin/admin` is used until first configuration.
- Portable settings backup excludes credentials by design; this is not a complete flash recovery image.

## Release blockers
1. **High:** No verified clean-install factory image, flash offsets, chip/flash-size compatibility check, or recovery demonstration.
2. **First-run design (accepted):** `admin/admin` is an intentional bootstrap credential. Source review confirms `forcePasswordChange` redirects authenticated users to `/account` before other protected settings are accessible, and the password-change form requires a new password of at least 12 characters. **Residual risks to review separately:** exposure before provisioning, fixed setup AP credentials, and HTTP Basic authentication without transport encryption on the LAN. Do not classify the existence of the factory credentials alone as a defect.
3. **High:** Wi-Fi credentials and YouTube API key are stored in ESP32 Preferences/NVS; a raw flash dump can expose them. No evidence of NVS encryption in this baseline.
4. **High:** Third-party licenses and bundled assets remain unverified; `XPT2046_Touchscreen` is referenced by unpinned Git URL.
5. **Medium:** The root LICENSE references the official PolyForm text but does not include the full verbatim license; documentation licensing has its own notice.
6. **Medium:** Existing tests mainly check source invariants. They do not establish runtime authentication, memory safety, long-run stability, or factory flashing.
7. **Medium:** CI does not publish hashes, resolved dependency lock/SBOM, or a merged image.
8. **Medium:** The current GitHub Pages deploy runs only on installer-web/workflow changes; firmware commits do not redeploy Pages, by design.

## Review limitations
- PlatformIO compiler unavailable locally; CI success was shown in the user's screenshot for commit `8aefb83`, but this archive was not independently matched to that commit.
- No access to a connected CYD for flash readout, hardware tests, or flash size confirmation.
- Static credential pattern scan is not a comprehensive secret scan.
- License conclusions are preliminary and not legal advice.

## Secret scan
No matches for selected private-key / Google API key / likely hardcoded-credential patterns in source files.

**Do not publish raw flash dumps, local NVS exports, serial logs containing secrets, or private backups.**
