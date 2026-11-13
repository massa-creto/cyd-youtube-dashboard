# Wi-Fi recovery access point

When the saved AP preference is enabled, the CYD operates in AP+STA mode as before.
When the saved AP preference is disabled, the CYD normally operates in STA mode.
If STA remains disconnected for 60 seconds, firmware temporarily enables the
configured AP (`apName`, `apPassword`, channel, visibility). The web configuration
server remains available on the AP interface, normally at `192.168.4.1`.
When STA reconnects, this temporary AP is stopped. The saved AP preference is
not changed. If the saved AP preference is enabled, the AP stays enabled.

**Privacy/security:** Recovery AP uses the user's existing AP security settings.
For production, use a unique strong WPA2 password and avoid open AP mode. Factory
credentials are intended for first-run provisioning only; change them before use.
The AP may be hidden if configured hidden, so an apparently missing SSID may
still be available. A power-cycle does not clear stored network settings.

**Validation pending:** Confirm the 60-second timeout, repeated disconnections,
AP availability, reconfiguration, and reconnection on a real ESP32 CYD. No
firmware flash or erase is required to exercise this feature after installation.
