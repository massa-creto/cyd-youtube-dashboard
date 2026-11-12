# CYD YouTube Dashboard

**A little screen for the numbers you keep checking.**

![How the dashboard works](docs/assets/architecture.svg)

If you run a YouTube channel, you've probably checked your subscriber count more often than you'd care to admit. This project puts public channel statistics on a small, inexpensive ESP32 touchscreen so you can leave your browser tabs alone.

It's a standalone device: once connected to Wi-Fi and configured with your own YouTube Data API key, the CYD fetches channel data and displays it locally. The built-in web dashboard handles setup, appearance, diagnostics and maintenance.

> **Project status:** Development candidate (RC6.1 firmware baseline). Source builds and device uploads have been tested on the maintainer's ST7789 CYD. The one-click factory USB installer is **not released yet**; its image and first-boot behavior still require validation. Please don't use an unverified installer image.

## What you'll need

| Item | Notes |
| --- | --- |
| ESP32 Cheap Yellow Display | **ESP32-2432S028R**, 2.8-inch 240×320 touchscreen, **ST7789** variant used for testing |
| USB data cable | A charging-only cable won't work for flashing |
| Computer | Windows, macOS or Linux for the PlatformIO route; Chrome/Edge with Web Serial for the future browser route |
| 2.4 GHz Wi-Fi | The ESP32 cannot join a 5 GHz-only network |
| Google account | To create your own YouTube Data API v3 key |
| Public YouTube channel | Your own channel or another public channel, subject to API availability |

**Check your board before flashing.** CYD boards look similar but may use different display controllers, touch wiring and flash layouts. This project currently targets the ST7789 configuration above; other variants are not automatically supported.

## Choose your installation route

### Option A — Build it yourself (available now)

1. Install [Visual Studio Code](https://code.visualstudio.com/) and the [PlatformIO IDE extension](https://platformio.org/install/ide?install=vscode).
2. Download or clone this repository, then open the **`firmware/`** folder as your PlatformIO project.
3. Connect the CYD using a USB data cable.
4. Run **PlatformIO → Build**. Check that the environment is `cyd_st7789`.
5. Run **Upload**, then **Monitor** at 115200 baud.
6. Follow [First-time setup](docs/USER_GUIDE.md) to configure the device.

The firmware configuration is in [`firmware/platformio.ini`](firmware/platformio.ini). If your display controller differs, do not flash this build blindly.

### Option B — Install from your browser (coming soon)

The [installer website](https://massa-creto.github.io/cyd-youtube-dashboard/) is hosted on GitHub Pages. Our planned USB installer uses ESP Web Tools, so visitors won't need an IDE or Python. **The factory-flash action stays disabled until we verify the generated binaries and flash offsets on a clean board.** The page explains the current status and links to the source installation guide.

A browser installation will require a compatible Chrome/Edge browser, a USB data cable and permission to access the serial port. It won't require a Google login to flash; each user configures their own API key afterwards.

## What it does

- Displays public YouTube channel statistics and channel artwork on the TFT.
- Provides a local web control panel for Wi-Fi, channel setup, colors and display settings.
- Offers a real TFT snapshot and remote touch from the browser (on demand, not video streaming).
- Includes RGB notifications, backup/restore options and quota information.
- Shows device uptime and a compact diagnostics view with a recent event history.

For a realistic list of implemented and unverified features, see the [feature matrix](docs/FEATURE_MATRIX.md).

## First-time setup and API safety

Each device owner creates **their own** Google Cloud project and YouTube Data API v3 key. We don't ship a shared key and the GitHub Pages site doesn't ask for your credentials.

Start with the [device setup guide](docs/USER_GUIDE.md), then follow the [Google API key guide](docs/GOOGLE_YOUTUBE_API_SETUP.md). Restrict the key to **YouTube Data API v3**, review quota limits, and avoid linking a billing account unless needed. Quota restrictions are useful but cannot guarantee that a leaked key won't consume the project's quota.

## How the pieces fit together

```text
YouTube Data API v3
        │ HTTPS
        ▼
 ESP32 + Wi-Fi + NVS ───── local HTTP admin dashboard
        │                         │
        ├── ST7789 TFT             ├── Settings / Diagnostics
        ├── XPT2046 touch          └── Snapshot / Remote touch
        └── RGB LED
```

The device stores Wi-Fi credentials and the API key locally in ESP32 NVS. The current firmware does **not** enable flash encryption. Read [Security](docs/SECURITY.md) before deploying on an untrusted network or giving away a configured device.

## Documentation

| Guide | Start here when… |
| --- | --- |
| [User guide](docs/USER_GUIDE.md) | You're setting up a device |
| [YouTube API key](docs/GOOGLE_YOUTUBE_API_SETUP.md) | You need a key and quota safeguards |
| [GitHub Pages and installer](docs/GITHUB_PAGES_AND_FLASHER.md) | You're deploying the project website |
| [Architecture](docs/ARCHITECTURE.md) | You want to understand the design |
| [Diagnostics](docs/DIAGNOSTICS.md) | You want to inspect device events |
| [Security](docs/SECURITY.md) | You want to understand current risks |
| [Release readiness](docs/RELEASE_READINESS.md) | You're checking what still needs testing |
| [Contributing](CONTRIBUTING.md) | You want to report a bug or improve the project |

## License and credits

Original firmware and repository tooling: [PolyForm Noncommercial 1.0.0](LICENSE). Original documentation: [CC BY-NC-SA 4.0](docs/LICENSE). This is **source-available for noncommercial use**, not OSI-approved open source. Third-party components retain their own licenses; see [notices](THIRD_PARTY_NOTICES.md).

Created by **[MassaCreto](https://www.youtube.com/@MassaCreto)**. The creator's channel name and artwork are not part of the firmware or licensed for reuse with the project.

Found a bug or have an idea? [Open an issue](https://github.com/massa-creto/cyd-youtube-dashboard/issues). Please remove Wi-Fi passwords, API keys and personal network information from screenshots and logs first.
