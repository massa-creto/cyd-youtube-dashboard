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
