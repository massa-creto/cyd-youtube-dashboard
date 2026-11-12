# RC6.1 — Sidebar uptime

The web settings sidebar now shows elapsed time since the ESP32 last booted. The value is obtained from the ESP32 monotonic timer through the authenticated `GET /api/system/uptime` endpoint. The browser updates the display once per second and resynchronizes every 30 seconds.

- Format: `HH:MM:SS` for uptime shorter than one day; `Nd HH:MM:SS` thereafter.
- The counter resets after device restart, not after refreshing the web page.
- No NVS writes or persistent uptime history.
- The endpoint requires existing admin authentication and disables HTTP caching.
- On small screens the sidebar footer remains visible beneath navigation.

**Validation:** Source-level tests only. PlatformIO build and physical-device verification remain required.
