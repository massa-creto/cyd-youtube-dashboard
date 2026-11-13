# Browser flash-capacity checkpoint

This checkpoint adds an **offline-only JEDEC ID decoder** to `/backup/`.
It does not probe the chip, read flash, upload a RAM stub, erase, write, or enable firmware installation.

## Why capacity is not automatically detected yet

The ESP32 ROM handshake and memory-register diagnostics do not establish external SPI flash size. A reliable automated probe requires a verified SPI flash identification implementation and device-specific transport tests. Do not equate ROM magic, USB VID/PID, partition metadata, or a manually entered JEDEC ID with proven physical capacity.

The decoder supports conventional capacity codes 0x14–0x19 (1–32 MiB). Its input is always marked **unverified** and cannot unlock backup or flashing. A read-only capacity probe and independent validation on the actual CYD remain future work.

## Safety gates before full backup

1. Confirm ROM download mode and actual chip identity on physical hardware.
2. Confirm SPI flash JEDEC ID with a reviewed transport implementation; handle unknown IDs and encrypted/secured devices conservatively.
3. Validate capacity against hardware and prevent over-read.
4. Read flash to local storage with chunk validation, integrity hash and explicit user confirmation.
5. Never send flash erase/write commands in read-only mode. RAM stub upload, if used, must be separately reviewed and disclosed.

## Known issue to review before hardware tests

The current ROM diagnostics use asynchronous Web Serial reads and timeouts. All timeout branches must cancel and await the pending reader before releasing its lock; verify reconnection behavior in Chrome/Edge. This checkpoint does not claim hardware-ready ROM diagnostics.
