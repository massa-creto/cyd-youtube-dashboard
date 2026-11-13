# Test and verification matrix

This table separates automated checks from physical-device evidence. A green CI build is not proof of compatibility with every CYD board.

| Check | Status | Evidence / limitation |
|---|---|---|
| Python source tests | Automated | `python -m unittest discover -s tests -v` |
| JavaScript installer tests | Automated | `node --test installer-web/backup/*.test.mjs installer-web/preflight/*.test.mjs installer-web/install/*.test.mjs` |
| PlatformIO firmware build | CI tested | GitHub Actions `firmware-build` job |
| Release binary SHA-256 | Validated by installer | `installer-web/release/release-metadata.json` |
| Browser USB identification | Hardware tested | ESP32-D0WD-V3, 4 MB, CH340 |
| Browser application write at `0x10000` | Hardware tested | 1,144,592-byte application write completed |
| Firmware boot and settings retention | Hardware tested | Maintainer confirmed device boot and retained settings |
| Local Python full-flash backup | Hardware tested | 4,194,304-byte read, SHA-256, downloaded files |
| Full three-image factory installation | Not hardware tested | Bootloader and partition write path requires validation |
| Browser-native full backup | Not available | Local Python companion required |
| Full-device restore | Disabled | Recovery workflow not hardware validated |
| Other CYD display revisions | Not verified | Do not assume hardware compatibility |
| Independent security assessment | Not completed | Review network provisioning and local secrets before deployment |

For release scope and caveats, see [public release status](PUBLIC_RELEASE_STATUS.md).
