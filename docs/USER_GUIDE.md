# First-time setup and installation

> **Installer status:** Browser application flashing and boot were tested on one ESP32-D0WD-V3 / 4 MB ST7789 CYD. Full three-image first-time installation and full-device restore are not hardware validated. Backups are optional but recommended.

## Requirements

- ESP32-2432S028R CYD with **ST7789** display, USB data cable, 2.4 GHz Wi-Fi.
- Windows 11, Linux or macOS with PlatformIO for source builds. Chrome or Edge on desktop is required for browser installation.
- Personal Google account and restricted YouTube Data API v3 key (see [API guide](GOOGLE_YOUTUBE_API_SETUP.md)).

## Build from source

1. Install VS Code and PlatformIO or the PlatformIO CLI.
2. Download the repository and open the `firmware/` directory.
3. Run `pio run -d firmware -e cyd_st7789` from the repository root.
4. Confirm compilation succeeds. Flash only using PlatformIO with the correct USB port and known matching board; review the [browser installer](https://massa-creto.github.io/cyd-youtube-dashboard/install/) safety warnings before flashing. Full three-image installation is not yet hardware validated.
5. On a newly flashed device, join Wi-Fi AP **`CYD-YOUTUBE`**, password **`creator1234`**.
6. Open **`http://192.168.4.1`** and authenticate using **`admin` / `admin`** for first-time setup. Set a unique admin password of at least 12 characters immediately.
7. Change the AP password to a unique 12+ character password or disable the AP after station Wi-Fi works. Configure home 2.4 GHz Wi-Fi.
8. Configure your own API key and channel ID. Check metrics and refresh behavior.
9. Keep the device on a trusted LAN; never forward port 80.

**Caution:** Existing settings may survive firmware updates; factory reset / erased NVS removes them. Changing backup schema to `cyd-youtube-settings` means old branded backup JSON is **not** imported automatically; create a fresh backup before updating.

## Troubleshooting

- Blank display: confirm ST7789 hardware variant, backlight and wiring.
- No Wi-Fi: confirm 2.4 GHz SSID and password; reset to AP mode if needed.
- YouTube errors: check channel ID, API key restrictions, quota, Wi-Fi and clock synchronization.
- TLS failures: inspect clock synchronization and CA validity; do not disable certificate validation.
