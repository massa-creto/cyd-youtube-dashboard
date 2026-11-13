# GitHub Pages and browser flashing

There are two separate ways to install this firmware.

## Build from source — available

Follow the [README](../README.md) or [user guide](USER_GUIDE.md). Open `firmware/` in PlatformIO, build the `cyd_st7789` environment, upload and configure the device. This is the route currently supported by a tested upload.

## Browser flashing — planned, not yet enabled

The Pages site is a documentation and download landing page. It deliberately does **not** show an active flash button. A factory installer requires the exact bootloader, partition table, boot application support file (where applicable) and application image, with offsets verified against the built configuration and an actual clean CYD.

The repository contains `tools/make_web_release.py` as a preliminary **app-only upgrade** asset generator. It is **not** a factory flasher. An app-only image does not install a bootloader or partition table, and must not be presented as a fresh-device installation method. Do not run it on an unknown board.

When the factory image has passed validation, we can enable an ESP Web Tools button that uses a versioned manifest and checksums. A separate upgrade option should preserve NVS, while clean installation may erase existing settings.

## Publish the website

1. Push the repository contents to the `main` branch, including `.github/workflows/pages.yml` and `installer-web/`.
2. In **Settings → Pages**, select **GitHub Actions** as the build and deployment source.
3. Under **Actions**, run **Build, test and publish safe installer landing page** (or push a change under `installer-web/`).
4. Wait for the workflow to succeed, then visit https://massa-creto.github.io/cyd-youtube-dashboard/.
5. Check the [build workflow](https://github.com/massa-creto/cyd-youtube-dashboard/actions) separately. A successful CI build is necessary, but not sufficient, to approve factory flashing.

## Release gate for the USB flasher

- Build firmware and verify image sizes against the real partition layout.
- Confirm flash offsets and bootloader/boot-app files from the exact toolchain.
- Test on an erased, supported ST7789 CYD and verify first-boot setup.
- Test upgrade on an already configured device without losing NVS.
- Verify HTTPS access to Pages, Web Serial in supported browsers, and failure messages.
- Publish checksums and document recovery steps.

Do not publish a guessed manifest or upload binary files copied from an unrelated board configuration.

## Safety and recovery

The browser-first release requires a full flash backup even for new devices, local verification and explicit acknowledgement of brick/data-loss risks before any write. These functions are **not yet implemented or hardware validated**. See [web flasher safety requirements](WEB_FLASHER_SAFETY_AND_RECOVERY.md) and the [public recovery guide](../installer-web/recovery/).
