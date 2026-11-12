# RC7 handoff

This package is based on the RC6.1 firmware baseline (diagnostics and sidebar uptime). The firmware source is carried forward without functional changes. The public README, GitHub Pages landing page, installation-path documentation and upload guide were refreshed.

**Ready:** source distribution, PlatformIO instructions, GitHub Pages informational site, GitHub Actions build/test workflow.

**Not ready:** one-click factory flash, verified clean-install images, hardware validation of generated release binaries, production security claims.

Do not upload local flash dumps or secrets. The factory installer should only be activated after a verified, clean-device flash test.

## GitHub Actions LEDC compatibility patch

The firmware now supports both Arduino-ESP32 2.x channel-based LEDC and
Arduino-ESP32 3.x pin-based LEDC. `initPwm`, `writeLedPwm`, and
`writeBacklightPwm` keep the RGB indicator and TFT backlight behavior
consistent across these APIs. The PlatformIO Espressif32 platform is pinned
to 6.12.0 for reproducible CI dependency resolution.

Run `python -m unittest discover -s tests -v` and
`pio run -d firmware -e cyd_st7789` before publishing a firmware binary.
The compiler build and hardware behavior still require verification in GitHub
Actions and on the actual CYD board.
