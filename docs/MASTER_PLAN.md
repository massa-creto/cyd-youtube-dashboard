# CYD YouTube Dashboard — Web Flasher & Public Release master plan

**Baseline:** user-provided RC7 GitHub Build Fix source archive; firmware kept unchanged.
**Target:** ESP32-2432S028R / ST7789 only until additional variants are separately verified.
**Safety:** do not flash the working CYD during early development; no `erase_flash` without backup and explicit approval.

| Session | Objective | Completion gate |
|---|---|---|
| 1 | Baseline audit and freeze | Static audit, tests, risks and checkpoint documented |
| 2 | Factory binary pipeline and backup | Exact offsets, flash size, checksums, recovery plan verified |
| 3 | Read-only Web Serial diagnostics | No flash write/erase paths; mock/browser tests pass |
| 4 | Install-capable Web Flasher | Consent, compatibility, error handling; not publicly enabled |
| 5 | First-run onboarding | Clean device Wi-Fi, admin password, YouTube setup verified |
| 6 | Security, recovery, regression | Recovery demonstrated and no unresolved critical defects |
| 7 | Documentation and GitHub Pages | Accessible polished site and complete support docs |
| 8 | Release candidate and publication | Hardware validation, CI artifacts, explicit release approval |

Each session produces a full source checkpoint, test results, updated matrices, and a handoff. Never represent untested hardware behavior as verified.

## Session 2 checkpoint (2026-11-13)
Packaging helpers, partition parser, read-only Windows backup script and unit tests added. **Review-only** artifacts; factory image remains blocked pending actual build partition inspection, device flash size, and spare-device recovery validation. See `SESSION2_PACKAGING.md` and `SESSION2_TEST_RESULTS.md`.

## Session 3 checkpoint (completed in source; hardware pending)
Read-only USB VID/PID diagnostics, simulation and no-write safeguards. This is NOT an ESP32 chip identity or flash-size verification. Factory flashing remains disabled. Session 4 depends on validated build artifacts and recovery testing.

## Session 4 checkpoint
Implemented offline integrity validation and a static installation readiness page. No factory manifest, flashing code, erase commands, or device writes. Hardware release gates remain open.

## Current checkpoint — factory validation and onboarding preparation
Offline artifact validation and first-run acceptance protocol are prepared. No actual build artifact or spare hardware was provided, so no factory installation or recovery test was performed.
