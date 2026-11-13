# Verification matrix

| Check | Result | Evidence |
|---|---|---|
| Static unit/source checks | Run locally | `python -m unittest discover -s tests -v` |
| Brand removal from firmware | Run locally | Case-insensitive source scan |
| Docs link existence | Run locally | Repository link checker |
| ZIP integrity | Run locally | Python `zipfile.testzip()` |
| PlatformIO compilation | Not run in this environment | GitHub Actions required |
| Firmware image and partition offsets | Not verified | Factory flashing blocked |
| Clean-device flash | Not run | Physical CYD required |
| HTTPS Google API and avatar | Not run | Real Wi-Fi/credentials required |
| Browser, TFT snapshot, touch | Not run | Physical CYD and browser required |
| Penetration/security assessment | Not run | HTTP admin, NVS and provisioning remain risks |
| Exact dependency licenses | Partial | Resolve/pin/build and audit |


## Session 1 audit checkpoint (2026-11-13)

- PASS: `python -m unittest discover -s tests -q` — 42 tests.
- STATIC: PlatformIO config, CI workflow, Pages workflow, source security review.
- NOT RUN: PlatformIO local compilation (toolchain unavailable).
- NOT RUN: USB diagnostics, factory flashing, clean-device setup, hardware recovery.
- EXTERNAL EVIDENCE: User screenshot reports GitHub Actions firmware build pass for `8aefb83`; archive-to-commit identity not verified.

## Session 2 additional tests
- Partition parser normal, overlapping, invalid entries: 3/3 passed.
- All Python unit tests: 45/45 passed.
- Python syntax compilation: passed.
- Firmware source unchanged: SHA-256 matched.
- Real build partition validation, esptool backup, restore, factory flashing: **NOT TESTED**.

## Session 3
- Automated: no `port.open`, serial write, flash/erase or install methods in diagnostics script; landing page link and CSP assertions.
- Manual pending: Chrome/Edge USB chooser, real USB VID/PID, unplug and permission behavior, device compatibility.
- Firmware compile: not performed in Session 3.

## Session 4 (2026-11-13)
- 52/52 Python tests passed (four new installation safety tests).
- JS syntax checked for existing read-only diagnostics.
- Firmware SHA-256 matches Session 3 archive exactly.
- Review-only candidate verifier checks file lengths, hashes, source commit and required metadata.
- No real compiled artifacts, actual flash offsets, factory manifest, spare-board recovery or live browser tests available.

## Factory validation preparation
- Offline artifact review unit tests added.
- First-run checklist authored; all hardware rows PENDING.
- No firmware flash, erase, partition changes or hardware tests executed.
