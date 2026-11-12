# YouTube Data API v3 — user setup and cost safeguards

Official pages: https://console.cloud.google.com/ ; https://docs.cloud.google.com/docs/authentication/api-keys ; https://developers.google.com/youtube/data/determine_quota_cost

## Create a dedicated project and API key
1. Sign in to https://console.cloud.google.com/ using your own Google account.
2. Open project selector > **New Project**; give it a unique name such as `My-CYD-YouTube-Stats` and select the project.
3. Navigate to **APIs & Services > Library**, search **YouTube Data API v3**, open it and select **Enable**.
4. Go to **APIs & Services > Credentials > Create credentials > API key**. Copy the key privately; do not post it in issues, screenshots, GitHub, or source code.
5. Open the key settings. Under **API restrictions**, select **Restrict key** and choose **YouTube Data API v3 only**; save.
6. **Application restrictions** are difficult for direct ESP32 requests: website referrer restrictions usually break device-origin requests; Android/iOS restrictions are inapplicable. IP restrictions work only with a stable public outbound IP and may break on ISP changes, mobile networks, or CGNAT. If no reliable IP restriction exists, use **None** for application restrictions and rely on API restriction + quotas. This is a documented residual risk, not proof the key is safe if leaked.
7. Open **APIs & Services > Enabled APIs & services > YouTube Data API v3 > Quotas & System Limits**. Inspect quota units and lower editable daily limits to a value compatible with device usage, where Google allows editing. The default combined allocation for most endpoints is 10,000 units/day, but check the console.
8. For a five-minute refresh with three 1-unit API calls, expected usage is about **864 units/day** (plus manual refreshes, retries and other calls). A modest limit such as **1,500–2,000 units/day** may fit one device if the console allows it. This is an example, not a universal guarantee. If the project quota is reached, updates will stop until quota resets or increases.
9. Under **Billing**, avoid linking a billing account unless Google explicitly requires it and you accept the implications. If billing is linked, configure **Budgets & alerts** and review service costs; **alerts are not a hard spending cap**. If eligible, use an actual spend-cap budget, but verify supported services and its limitations.
10. Use **APIs & Services > Metrics** and Quotas to monitor real consumption; the CYD's on-device counter is an estimate, not Google's official usage.
11. In the CYD web settings, paste your API key and channel `UC...` ID or handle; save and refresh. Public data needs an API key, not OAuth. YouTube Studio private analytics require OAuth and are not provided by this firmware.
12. If exposed, **rotate or delete the API key** in Google Cloud and replace it on the device. API restriction limits which APIs a leaked key can call, but does not stop someone consuming the same YouTube quota.

## About charges
The YouTube Data API uses quotas. A default quota allocation is **not** a guarantee that all Google Cloud services are free, nor a hard billing guarantee. Isolate the project, enable only YouTube Data API v3, set quotas, avoid unnecessary billing and monitor the project. Do not promise zero financial risk when a billing account is attached.
