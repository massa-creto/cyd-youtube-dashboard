# Diagnostics and compact appearance editor

## Appearance

The desktop Appearance page presents web colors, TFT colors and RGB LED alerts in a three-column layout. At narrower viewport widths the columns collapse to two and then one. This is a responsive layout, not a promise that every laptop screen will show the full panel without scrolling. Existing saved color and LED settings remain compatible.

## Diagnostics

The **Diagnostics** menu provides the most recent 12 informational, warning or error events, an on-demand refresh control, a clear-history action and a toggle for serial debug messages. The same controls are available through authenticated `GET /api/diagnostics` and `POST /api/diagnostics` endpoints. POST accepts `action=settings&debug=1|0` or `action=clear`.

The event ring lives in RAM. A compact snapshot is written to the `cyd-diag` NVS namespace at most once per minute, except when the user explicitly clears it. Consequently, the last minute of events may be lost on sudden power failure. Timestamps are device uptime seconds, not wall-clock time. Events are capped at 112 characters and avoid API keys, Wi-Fi passwords, request URLs and response bodies.

Serial debug logging is off by default. Turning it on does not enable remote network logging. Some pre-existing status messages still print to Serial regardless of the debug setting; the toggle applies to the new diagnostic debug messages. Do not publish unredacted serial logs or screenshots.

## Verification

The code changes require a PlatformIO build and device test. Verify layout at 1920×1080 and 1366×768; verify logging on/off, reboot persistence, event rollover, clearing and NVS settings migration. This revision has not been certified by a hardware test.
