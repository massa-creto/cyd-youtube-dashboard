# Browser dependency validation checkpoint

The Web Flasher identification workflow uses the pinned esptool-js v0.7.0 CDN bundle.
This checkpoint adds API-export validation, dependency-load timeout, sanitized error
messages, duplicate-session protection and disconnect cleanup. No flash operations
are enabled.

## Release blockers

1. Independently confirm the CDN URL resolves and exposes `window.esptooljs`
   with `ESPLoader` and `Transport`. A pinned version is not an integrity hash.
2. Prefer a locally vendored, integrity-verified bundle for production.
3. Validate on a real ESP32 CYD via Chrome/Edge on HTTPS. The connection may
   reset the device or load a RAM stub; it is not a flash write.
4. Test physical disconnect, browser permission denial, serial-port contention,
   unsupported chips, and repeat connections.
5. Before any backup: verify flash capacity using a supported API and validate
   readFlash compatibility. Before any write: independent backup and restore QA.

No successful browser/hardware validation is claimed in this checkpoint.
