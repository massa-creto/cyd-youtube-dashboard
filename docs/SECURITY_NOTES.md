# Security notes — Session 1

## Trust boundaries
1. Public GitHub source and Pages must contain no user secrets.
2. Browser installer uses Web Serial only with explicit user interaction and device confirmation.
3. Local device administration uses HTTP Basic auth; use only on a trusted network. Credentials can be exposed to local network observers.
4. Preferences/NVS contains SSID, Wi-Fi password, API key, AP password, and admin password hash/salt. Raw flash backups are sensitive.
5. YouTube HTTPS requests must retain CA validation and clock readiness checks.

## Required before public installer
- Never bundle API keys, Wi-Fi credentials, or personal channel assets into factory binaries.
- Preserve the intentional factory `admin/admin` bootstrap and mandatory first-login password change. Verify endpoint enforcement, post-change invalidation, and first-boot AP exposure separately; do not flag the bootstrap credential itself as a defect.
- Add origin/auth/CSRF and endpoint regression tests.
- Restrict Web Serial flashing to confirmed supported chip/flash layout; provide backup and recovery guide.
- Keep flash backups local; do not upload them to GitHub or CI.
- Validate release binary provenance and hashes.

## Explicit non-goals in Session 1
No credential migration, NVS encryption changes, partition changes, or bootloader updates.
