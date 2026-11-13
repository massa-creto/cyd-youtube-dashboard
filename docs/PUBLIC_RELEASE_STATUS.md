# Public release status

## Verified on physical hardware

- Chrome/Edge Web Serial connection via CH340 and original ESP32 identification.
- 4 MB flash capacity / JEDEC identification.
- Browser application write of 1,144,592 bytes at 0x10000, completed without reported error.
- Firmware booted and retained previous settings after the browser write (maintainer report).
- Full 4 MB read-only backup using local Python esptool companion; SHA-256 generated and both files downloaded.

## Not yet validated

- First-time full three-image installation (bootloader, partition table and application).
- Browser-native full flash backup without Python.
- Browser restore of full 4 MB backups (disabled).
- Recovery from a failed flash on a physical board.
- Other CYD display controllers, flash sizes and hardware revisions.
- Accurate intermediate backup progress: the helper may show 0% until complete.

## User-facing safety

Backup is optional. Without a supplied backup, the installer uses a full three-image write. This is a more invasive path and is not hardware validated. Users must explicitly confirm the risk. Full backups may contain private credentials.

This release includes a tested application-write path, but is not a guarantee of compatibility or recoverability.
