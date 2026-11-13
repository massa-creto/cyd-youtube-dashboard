# ROM chip identification checkpoint

The Web Serial page now requires an explicit ROM SYNC followed by a user-triggered read of ROM magic at `0x40001000` before enabling the original-ESP32-specific eFuse register diagnostic. Only magic `0x00f01d83` is recognized as original ESP32; all other values are treated as unknown.

This is **not** proof of the precise CYD board revision, flash capacity, flash encryption state, or backup capability. No SPI flash read, erase, or write command is implemented here. Browser flashing remains disabled.

Hardware verification is pending. The ROM read may time out if the board is not in download mode. Serial port opening can reset some boards.

Do not use this diagnostic as authorization to flash firmware.
