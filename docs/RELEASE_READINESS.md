# Release readiness

**Status: development candidate. Do not enable public browser flashing or advertise a production release.**

## Evidence available

- RC3.1 was built, uploaded and booted by the project owner using PlatformIO.
- Serial output showed Wi-Fi connectivity, YouTube API HTTP 200 responses and a successful avatar download.
- RC4 source checks were reported as passing locally. These checks primarily inspect source invariants; they do not replace compilation or hardware QA.
- GitHub Pages currently serves a static project page without flash capability.

## Release blockers

| Area | Required evidence | State |
| --- | --- | --- |
| Reproducible build | Successful GitHub Actions build, recorded dependencies and artifact hashes | Open |
| License compliance | Verbatim applicable licenses, embedded asset review, complete binary dependency notices | Open |
| First-run security | No universal administrative credential or unsafe provisioning exposure | Open |
| Transport security | Google TLS certificate validation and clock synchronization tested under failure conditions | Open |
| Local secrets | Documented threat model for NVS and API key exposure; safe reset path | Open |
| Hardware QA | Touch, display, channel refresh, snapshots, backup/restore, reboot and long-run tests | Open |
| Factory flash | Verified bootloader, partition table, boot_app0, application offsets and clean-device boot | Open |
| Upgrade | Verified update procedure preserving user settings, with rollback guidance | Open |
| Web installer | Flash manifest built only from tested artifacts and correct offsets | Open |

## Release procedure

1. Complete the dependency and licensing inventory for the exact resolved build.
2. Run CI and capture the successful build logs and binary SHA-256 hashes.
3. Test a clean factory installation on the supported ST7789 board.
4. Exercise the full [test matrix](TEST_MATRIX.md), including a configured-device upgrade.
5. Resolve or explicitly accept remaining security risks before distributing a binary.
6. Only then enable browser flashing and publish a tagged release.

The first public source commit is not the same thing as a tested firmware release.


## RC7 GitHub packaging note

The public README and Pages site now offer two installation paths, with the browser path clearly marked pending factory-image validation. This packaging pass does not add a tested factory flasher. RC6.1 firmware remains the baseline.
