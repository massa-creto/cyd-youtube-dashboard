# Session 3 — Browser USB Detection & Diagnostics

## Implemented
- Separate responsive `/diagnostics/` page linked from the existing GitHub Pages landing page.
- HTTPS/localhost and Web Serial capability checks, explicit user-initiated serial chooser.
- USB VID/PID display and limited USB-UART bridge hints (CP210x, CH340, FTDI).
- Offline simulation mode, local reset, USB-disconnect handling and redacted report copy.
- Strict Content Security Policy with `connect-src 'none'`; no external assets, credentials, or analytics.
- No port opening, no serial transmission, no chip ROM commands, no read/write/erase, no installer manifest.

## Important limitations
A USB bridge VID/PID does not identify the attached ESP32 chip or ST7789 display. Flash capacity, partition layout, board revision and firmware compatibility remain **unverified**. The browser's chooser may show serial devices other than CYD; selecting one does not establish compatibility. `Forget selected port` clears the page reference; it does not revoke the browser's persisted permission. Permission can be revoked via browser site settings.

## Manual tests requiring real browser/device
1. Serve `installer-web/` over localhost or GitHub Pages HTTPS; open `/diagnostics/` in desktop Chrome/Edge.
2. Confirm browser support status, select CYD USB serial port and inspect VID/PID.
3. Verify no device reset, no firmware write, and that existing dashboard keeps operating. Note: USB connection and OS behavior vary.
4. Cancel chooser; test permission denial, unplug, reconnect, and non-Web-Serial browser.
5. Run simulation without hardware and verify explicit SIMULATED label.
6. Inspect browser Network and Serial panels; no uploads or serial port opens expected.

## Security review
No firmware update is possible in this session. The UI intentionally does not claim to verify the chip or flash layout. Do not add automatic serial opening or chip detection without a separate protocol review and user consent. Public install remains disabled.
