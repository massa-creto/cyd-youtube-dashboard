# Web flash probing — safety gate

This checkpoint adds a privacy-preserving, downloadable browser diagnostic readiness report. It does **not** probe flash memory or determine device flash size automatically.

## Why flash probing is deferred

The ESP32 ROM SYNC and READ_REG operations do not expose a portable flash-capacity query. A production readback implementation requires a validated transport (typically a RAM-resident helper/stub) and hardware verification. A stub changes device RAM and execution state, even though it need not modify persistent flash. No RAM stub is uploaded in this checkpoint.

## Required next steps

1. Validate ROM SYNC and chip identification on the user's board through HTTPS/Web Serial.
2. Review a version-pinned ESP32 ROM/stub transport implementation and its licensing.
3. Implement and test read-only JEDEC probe and bounded flash readback; reject unknown chip and capacity.
4. Independently verify image size/hash and user-controlled local save; avoid storing images in browser storage or sending them to servers.
5. Keep installation/restore disabled until hardware validation, backup verification, and recovery rehearsal.

The readiness report deliberately excludes MAC addresses, raw register contents, secrets and serial logs. Passing readiness checks never enables flash writes.
