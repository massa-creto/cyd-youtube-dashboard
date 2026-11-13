# CYD Web Flasher

This folder is the static GitHub Pages site. Open `/install/` for the Web Serial installer, `/recovery/` for recovery guidance, and `/local-backup/` for the optional backup UI (which requires the separate localhost Python companion).

The application-only write and boot were tested on one original ESP32-D0WD-V3 / 4 MB / ST7789 CYD. Full three-image installation has not been hardware verified. Browser restore is disabled.

The install page validates hashes in `release/release-metadata.json` before writing. Keep `release/*.bin` and metadata synchronized. Do not upload private backups. The esptool-js browser bundle is loaded from a version-pinned CDN.
