# Release matrix — Session 1

| Area | Status | Evidence / next gate |
|---|---|---|
| ST7789 firmware source | Baseline frozen | Source present; hardware validation previously reported by owner |
| CI Python tests | PASS | 42/42 local tests |
| CI firmware build | Reported PASS | Screenshot for commit `8aefb83`; verify archive matches commit |
| GitHub Pages | Existing landing page | No flashing capability |
| YouTube TLS | Implemented, partially tested | Certificate failure and time-sync regression pending |
| Wi-Fi / YouTube provisioning | Implemented | Fresh-device end-to-end test pending |
| Authentication | Implemented, security review open | HTTP Basic / factory credentials risk |
| Portable backup/restore | Implemented, hardware QA open | Excludes secrets; does not restore raw flash |
| TFT snapshot / remote touch | Implemented, hardware QA open | Test device-dependent pixel decoding |
| RGB alert control | Implemented, hardware QA open | Confirm on target device |
| Firmware artifacts | Review-only | No merged verified factory binary |
| Browser USB detection | Not implemented | Session 3 |
| Factory browser flashing | BLOCKED | Session 4 + hardware gates |
| License compliance | BLOCKED | Capture exact third-party licenses and notices |
| Public stable release | BLOCKED | All critical gates must pass |
