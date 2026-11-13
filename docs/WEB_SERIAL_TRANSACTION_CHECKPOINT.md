# Web Serial ROM transaction checkpoint

This checkpoint replaces Promise.race-based response waiting in ROM SYNC, chip identification, and register diagnostics with a shared, bounded read transaction.

## Behavior
- User-initiated ROM operations only; no automatic probing.
- Reader cancellation on timeout and lock cleanup after success, timeout, or stream close.
- Strict 115200 baud connection; unknown chip variants remain gated.
- No SPI flash JEDEC probing, RAM stub upload, flash read, erase, write, or restore.
- A timeout can only be enforced after the serial writer.write() call completes. A disconnected USB device or browser/driver issue may still require unplugging/reconnecting.

## Hardware validation still required
Test on the exact ESP32-2432S028R board in Chromium Web Serial over HTTPS, including ROM download mode, disconnected cable, busy port, timeout, and reconnect. Do not enable firmware flashing based on unit tests alone.
