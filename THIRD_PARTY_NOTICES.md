# Third-party dependency inventory — incomplete verification

The project's PolyForm license applies **only** to original project code; dependencies retain their own upstream terms. The exact license text and transitive dependencies of a resolved build must be archived before binary distribution.

| Dependency | Requested version | Upstream | License verification status |
|---|---|---|---|
| TFT_eSPI | 2.5.43 | https://github.com/Bodmer/TFT_eSPI | Mixed notices and font assets; exact packaged notices **not yet archived** |
| XPT2046_Touchscreen | Git branch, unpinned | https://github.com/PaulStoffregen/XPT2046_Touchscreen | **Pin commit** and capture license |
| ArduinoJson | 6.21.5 | https://github.com/bblanchon/ArduinoJson | MIT; capture exact version notice |
| TJpg_Decoder | 1.1.0 | https://github.com/Bodmer/TJpg_Decoder | Check decoder core and embedded ChaN notices |
| Arduino-ESP32 / ESP-IDF | PlatformIO resolved | https://github.com/espressif/arduino-esp32 | Mixed upstream licenses; binary SBOM pending |
| ESP Web Tools | Not yet embedded | https://github.com/esphome/esp-web-tools | Not part of current installer landing |

**Do not interpret this inventory as completed legal compliance.**
