# Data storage and privacy

The device stores Wi-Fi credentials, YouTube API key, selected channel ID and administrator account state in ESP32 NVS. API key and Wi-Fi password are **not encrypted at rest** using hardware flash encryption. Administrator password is stored as a salted derived hash in current firmware. Factory credentials are public and must be changed on first use.

The safe backup feature intentionally excludes Wi-Fi passwords, API keys, admin credentials, history and quota counters. Exported backup schema is `cyd-youtube-settings` version 1. Historical branded backup schemas are no longer imported in this release candidate.

Google API requests send the user's API key to Google over certificate-verified HTTPS. Never share NVS partitions, debug serial dumps or configuration screenshots containing secrets.
