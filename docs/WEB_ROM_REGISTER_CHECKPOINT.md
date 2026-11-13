# Browser ROM register diagnostic — checkpoint

The backup page now offers an explicitly user-triggered ESP32 ROM `READ_REG` command (`0x0A`) after a successful `SYNC` (`0x08`). The fixed address `0x3FF5A004` is an ESP32-classic eFuse register. This is a **register diagnostic only**: it does not access flash memory, determine flash capacity, or guarantee support for other ESP32 variants. Do not use it as a model-identification or backup result.

## Safety and limitations

- Flash write, erase, restore and real backup remain disabled.
- Browser serial-port opening can reset the device; ROM download mode is required.
- Do not use the register diagnostic on an unknown chip variant: the fixed register address is specific to original ESP32, not ESP32-S2/S3/C3.
- A ROM response proves only communication with a bootloader; actual board identity, flash size and backup must be separately validated.
- No hardware test has been performed; tests use synthetic protocol responses.
- The existing firmware files are unchanged.

## Next step

Validate the ROM handshake on a real original ESP32 CYD; add robust device-family identification and only then attempt flash-size discovery. Full flash backup requires a flash-reading stub/transport and independent verification. The ROM register read itself does not back up flash.
