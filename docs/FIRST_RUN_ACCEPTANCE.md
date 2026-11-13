# First-run acceptance checklist

This is a manual acceptance protocol, not a claim of completed hardware testing. Perform only after the factory image and recovery procedure have been verified on a spare CYD.

| Test | Expected behavior | Evidence |
|---|---|---|
| Clean boot | Device starts with supported ST7789 orientation and touch | Photo and serial log without secrets |
| Setup AP | Default setup AP can be joined with documented factory settings | Connection and AP status |
| Factory admin login | `admin/admin` permits initial login | Authentication result |
| Mandatory password change | Other protected settings redirect to `/account` until changed | HTTP status and screenshot |
| Password policy | Rejects passwords shorter than 12 characters and mismatched confirmation | HTTP 400 responses |
| Credentials persistence | New username/password survive restart; factory credentials no longer work | Login checks |
| Wi-Fi provisioning | Joins 2.4 GHz LAN and shows current local IP | Screen and network status |
| YouTube configuration | User enters own restricted Data API v3 key and channel ID | Public channel statistics load |
| Restart | Wi-Fi and YouTube settings persist; channel branding is fetched | Repeat boot |
| AP control | User can disable setup AP after successful provisioning | Scan and settings |
| Backup privacy | Export excludes passwords/API keys; raw flash backup stays private | Local inspection |
| Recovery | Spare device can be restored from verified backup | Recovery log |

Do not upload credentials, real flash backups, API keys, or full serial dumps containing secrets. A test is **PENDING** until the maintainer records direct hardware evidence.

## Known security boundaries

`admin/admin` is the intentional bootstrap credential. Source inspection shows mandatory password change for protected settings. The first-boot exposure window, local HTTP Basic transport, and endpoint-by-endpoint authorization remain separate security considerations. No changes to authentication behavior are made in this milestone.
