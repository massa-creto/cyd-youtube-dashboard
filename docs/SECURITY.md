# Security and deployment guidance

## What is currently protected

- Google HTTPS requests use a bundled Google Trust Services root CA set and require synchronized time.
- Admin password is stored as a salted PBKDF2-derived hash, not plaintext (legacy migration path exists).
- Authenticated settings endpoints and Origin checks are present in source; they are **not** a substitute for a penetration test.

## Known risks (release blockers)

1. **HTTP Basic authentication on port 80:** credentials can be intercepted on an untrusted local network. Do not expose to WAN, public Wi-Fi or port forwarding.
2. **Universal factory credentials:** `admin/admin` and AP password `creator1234` are public. Change both on first setup; secure provisioning is pending.
3. **NVS secrets:** Wi-Fi credentials and Google API key are stored without flash encryption; physical access can expose them.
4. **API key in URL:** YouTube Data API key is included as a query parameter to Google over HTTPS. Avoid logging full URLs.
5. **TLS root lifecycle:** CA roots and device clock must be maintained; fail closed rather than bypass validation.
6. **Supply chain:** dependency commit pinning and full binary SBOM are incomplete.

## User precautions

Use a dedicated Google Cloud project and restrict the API key to YouTube Data API v3; set quota limits and do not assume budget alerts are hard spending caps. Never upload NVS dumps, API keys or passwords to GitHub issues. Treat this as experimental software until the release gates pass.

## Reporting

For sensitive security reports, contact the repository owner privately through an appropriate channel; do not open a public issue containing secrets or exploit details.
