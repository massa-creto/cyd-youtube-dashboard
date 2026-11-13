# Web ROM SYNC checkpoint

Status: **local software tests passed; physical CYD not tested**. Public flashing and real full backup remain disabled.

## What is implemented

- SLIP encode/decode and bounded frame collector.
- ESP32 ROM SYNC packet (`0x08`) and response parsing.
- Explicit button on `/backup/` for a ROM SYNC handshake after opening the serial port.
- A 2.5-second response timeout and user-facing status.
- No flash read, erase, write, partition changes, bootloader replacement, or reset commands.

## Important limitations

- SYNC sends bytes over USB; it is not a passive or strictly read-only serial operation. It does **not** request a flash modification.
- User must enter ESP32 ROM download mode using board-specific controls. Simply opening the serial port is insufficient.
- Successful SYNC does **not** establish the exact chip variant, flash capacity, partition layout, or recovery support.
- Web Serial requires a supported desktop browser, secure context, USB serial driver, and user permission.
- Some boards reset on port open due to DTR/RTS behavior. This is not a flash write, but can interrupt the running application.
- A timeout or failed response does not imply the device is bricked.
- Browser testing on actual hardware is required before enabling real flash reads.

## Hardware test checklist (no flash write)

1. Save current project files locally; do not publish or use the installer to flash.
2. Serve `installer-web/` on `http://localhost` or HTTPS. Open `/backup/` in desktop Chrome/Edge.
3. Close serial monitors, select the correct serial port, and click Connect.
4. Enter ROM download mode according to your board documentation, reconnect if necessary.
5. Click `Check ESP32 ROM handshake`. Record exact status and USB VID/PID.
6. Disconnect. Confirm normal boot after leaving download mode/resetting the board.

Do not upload full flash backups to GitHub. They can contain passwords, tokens, and private settings.

## Next engineering gate

Implement verified chip detection and flash size probing using a reviewed ESP32 protocol implementation, then implement chunked full-flash readback, local save, SHA-256, and independent verification. Do not enable write operations before backup and recovery have been hardware validated.
