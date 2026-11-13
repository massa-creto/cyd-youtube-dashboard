# Web Serial connection gate

The browser backup preview now includes a **connection-only** Web Serial permission check. It opens a user-selected serial port at 115200 baud, displays available USB VID/PID metadata, and closes it on request. It **does not transmit commands, enter ROM download mode, identify the ESP32, read flash, create a backup, erase flash, or install firmware**. USB VID/PID is not proof of board compatibility.

## Testing

- Python source tests: 59 passed.
- JavaScript tests: 9 passed, including permission, open/close, error handling and no-write mock coverage.
- No physical CYD or real browser tested. The test mocks do not prove hardware behavior.
- Firmware source remains unchanged.

## Next gate

Implement and validate ESP32 ROM transport and flash-read support, including safe handling of device reset/boot mode, verified flash size, interrupted reads, local save, integrity checks, and recovery limitations. Do not enable writing until a genuine, verified backup has been produced and restoration tested on appropriate hardware. A full flash dump can contain private credentials and must never be uploaded to CI or a public repository.

## Publishing

This is a local development checkpoint. The user may defer GitHub publishing until browser/hardware validation is needed. A public installer must continue to show flashing as disabled.
