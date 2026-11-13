#include <Arduino.h>
#include <esp_arduino_version.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <time.h>
#include <stdint.h>
#include <WebServer.h>
#include <Preferences.h>
#include <SPI.h>
#include <TFT_eSPI.h>
#include <XPT2046_Touchscreen.h>
#include <ArduinoJson.h>
#include <TJpg_Decoder.h>
#include <mbedtls/md.h>
#include <mbedtls/base64.h>
#include <esp_system.h>
#include <esp_timer.h>
#include "SecurityPolicy.h"
#include "ColorUtils.h"
#include "GoogleTrustRoots.h"

namespace {
constexpr uint8_t kBacklightPin = 21;
constexpr uint8_t kTouchCs = 33;
constexpr uint8_t kTouchIrq = 36;
constexpr uint8_t kTouchSck = 25;
constexpr uint8_t kTouchMosi = 32;
constexpr uint8_t kTouchMiso = 39;
constexpr uint32_t kPollMs = 300000;  // Default YouTube refresh: five minutes.
constexpr uint32_t kGlowFrameMs = 500;  // Five animation phases per 2.5-second cycle.
constexpr uint32_t kTouchDebounceMs = 260;
uint16_t kBackground = 0x0842;
uint16_t kCard = 0x1104;
uint16_t kGreen = 0x0488;  // #009245, RGB565 approximation
uint16_t kDarkGreen = 0x0326;
uint16_t kPurple = 0x6B5D;
uint16_t kWhite = TFT_WHITE;
uint16_t kMuted = 0xAD95;
uint16_t kLine = 0x2946;

// CYD ESP32-2432S028 RGB indicator: GPIO 4/16/17, active LOW on common boards.
// Some board revisions differ: test LED outputs before enabling alerts.
constexpr uint8_t kLedR = 4, kLedG = 16, kLedB = 17;
constexpr uint8_t kLedPins[3] = {kLedR,kLedG,kLedB};
constexpr uint8_t kLedChannel[3] = {1,2,3}; // PWM channel 0 is TFT backlight.
const char* const kThemeKeys[] = {"bg","card","green","darkgreen","purple","white","muted","line"};
uint16_t* const kThemeValues[] = {&kBackground,&kCard,&kGreen,&kDarkGreen,&kPurple,&kWhite,&kMuted,&kLine};
uint32_t webColors[5] = {0x0B1110,0x16241E,0x009245,0xFFFFFF,0x6B6BE8};
const char* const kWebKeys[5] = {"webbg","webcard","webaccent","webtext","websecondary"};
enum EventKind : uint8_t { SUB_EVENT, VIEW_EVENT, LIKE_EVENT, COMMENT_EVENT, EVENT_COUNT };
const char* const kEventKeys[EVENT_COUNT] = {"sub","view","like","comment"};
uint32_t eventColors[EVENT_COUNT] = {0x009245,0x6B6BE8,0xF0B44B,0x37B6DD};
bool eventEnabled[EVENT_COUNT] = {true,false,true,true};
bool ledEnabled = true;
uint8_t ledBrightness = 90; // 0-255
uint16_t ledDurationSeconds = 12;
uint32_t ledAlertUntil = 0;
int8_t activeLedEvent = -1;
uint64_t prevSubs = 0,prevViews = 0,prevLikes = 0,prevComments = 0;
bool havePrevCounters = false;
String prevChannelIdentity;
// Arduino-ESP32 2.x uses LEDC channels; 3.x binds PWM directly to GPIO pins.
void writeLedPwm(uint8_t index, uint8_t duty) {
#if ESP_ARDUINO_VERSION_MAJOR >= 3
  ledcWrite(kLedPins[index], duty);
#else
  ledcWrite(kLedChannel[index], duty);
#endif
}
void writeBacklightPwm(uint8_t duty) {
#if ESP_ARDUINO_VERSION_MAJOR >= 3
  ledcWrite(kBacklightPin, duty);
#else
  ledcWrite(0, duty);
#endif
}
void initPwm() {
#if ESP_ARDUINO_VERSION_MAJOR >= 3
  for (uint8_t i = 0; i < 3; ++i) ledcAttach(kLedPins[i], 5000, 8);
  ledcAttach(kBacklightPin, 5000, 8);
#else
  for (uint8_t i = 0; i < 3; ++i) {
    ledcSetup(kLedChannel[i], 5000, 8);
    ledcAttachPin(kLedPins[i], kLedChannel[i]);
  }
  ledcSetup(0, 5000, 8);
  ledcAttachPin(kBacklightPin, 0);
#endif
}
void ledOff() { for(uint8_t i=0;i<3;++i) writeLedPwm(i,255); }
void ledColor(uint32_t color, uint8_t level) {
  uint8_t rgb[3] = {uint8_t(color>>16),uint8_t(color>>8),uint8_t(color)};
  for(uint8_t i=0;i<3;++i) writeLedPwm(i,255-(uint16_t(rgb[i])*level/255));
}
void notifyEvent(EventKind event) {
  if(!ledEnabled || !eventEnabled[event]) return;
  activeLedEvent = event;
  ledAlertUntil = millis() + uint32_t(ledDurationSeconds)*1000UL;
}
void updateLed() {
  if(!ledEnabled || activeLedEvent<0 || (int32_t)(millis()-ledAlertUntil)>=0) {
    if(activeLedEvent>=0) { activeLedEvent=-1; ledOff(); }
    return;
  }
  const uint16_t t = (millis()/12)%512;
  const uint16_t triangle = t<256?t:511-t;
  const uint8_t intensity = uint8_t(uint16_t(ledBrightness)*triangle/255);
  ledColor(eventColors[activeLedEvent],intensity);
}

struct Metric {
  String label;
  String value;
  String change;
  int32_t unread;

  Metric(const char* label = "", const char* value = "",
         const char* change = "", int32_t unread = 0)
      : label(label), value(value), change(change), unread(unread) {}
};

struct Dashboard {
  String channel = "CHANNEL NOT LOADED";
  Metric subscribers{"SUBSCRIBERS", "--", "Waiting for YouTube", 0};
  Metric views{"TOTAL VIEWS", "--", "Waiting for YouTube", 0};
  Metric best{"TOP VIDEO", "--", "Waiting for YouTube", 0};
  Metric latest{"LATEST UPLOAD", "--", "Waiting for YouTube", 0};
  Metric likes{"LIKES", "--", "Waiting for YouTube", 0};
  Metric comments{"COMMENTS", "--", "Waiting for YouTube", 0};
  bool live = false;
  String updated = "Waiting for YouTube";
};

struct VideoStats {
  String id, title, published, duration;
  uint64_t views = 0, likes = 0, comments = 0;
  bool valid = false;
};
VideoStats latestVideo, topVideo;

// Forward declaration: overload must be visible before detail-page rendering.
String compactNumber(uint64_t n);

TFT_eSPI display;
SPIClass touchBus(HSPI);
XPT2046_Touchscreen touch(kTouchCs, kTouchIrq);
WebServer web(80);
Preferences prefs;
Dashboard data;

// Operational diagnostics: bounded RAM ring, with a small NVS snapshot for restarts.
// Debug output is opt-in; credentials, request URLs and response bodies are never logged.
constexpr uint8_t kDiagnosticCapacity = 12;
struct DiagnosticEntry { uint32_t uptimeSeconds; char level; String message; };
DiagnosticEntry diagnosticEntries[kDiagnosticCapacity];
uint8_t diagnosticCount = 0, diagnosticHead = 0;
bool debugEnabled = false;
bool diagnosticDirty = false;
uint32_t lastDiagnosticSave = 0;

void recordDiagnostic(char level, const String& message) {
  String safe = message.substring(0, 112);
  safe.replace("\n", " "); safe.replace("\r", " ");
  DiagnosticEntry& entry = diagnosticEntries[diagnosticHead];
  entry.uptimeSeconds = millis() / 1000;
  entry.level = level;
  entry.message = safe;
  diagnosticHead = (diagnosticHead + 1) % kDiagnosticCapacity;
  if (diagnosticCount < kDiagnosticCapacity) ++diagnosticCount;
  diagnosticDirty = true;
  if (debugEnabled) Serial.printf("[diag][%c][%lus] %s\n", level,
      (unsigned long)entry.uptimeSeconds, safe.c_str());
}
void debugDiagnostic(const String& message) {
  if (debugEnabled) Serial.printf("[debug][%lus] %s\n", (unsigned long)(millis()/1000), message.c_str());
}
void persistDiagnostics(bool force = false) {
  if (!diagnosticDirty || (!force && millis() - lastDiagnosticSave < 60000UL)) return;
  DynamicJsonDocument doc(3072);
  JsonArray entries = doc.to<JsonArray>();
  for (uint8_t i = 0; i < diagnosticCount; ++i) {
    const DiagnosticEntry& entry = diagnosticEntries[(diagnosticHead + kDiagnosticCapacity - diagnosticCount + i) % kDiagnosticCapacity];
    JsonObject item = entries.createNestedObject();
    item["t"] = entry.uptimeSeconds; item["l"] = String(entry.level); item["m"] = entry.message;
  }
  String serialized; serializeJson(doc, serialized);
  Preferences diagPrefs;
  if (diagPrefs.begin("cyd-diag", false)) {
    diagPrefs.putString("events", serialized);
    diagPrefs.end();
    diagnosticDirty = false;
    lastDiagnosticSave = millis();
  }
}
void loadDiagnostics() {
  Preferences diagPrefs;
  if (!diagPrefs.begin("cyd-diag", true)) return;
  debugEnabled = diagPrefs.getBool("debug", false);
  const String serialized = diagPrefs.getString("events", "[]");
  diagPrefs.end();
  DynamicJsonDocument doc(3072);
  if (deserializeJson(doc, serialized)) return;
  for (JsonObject item : doc.as<JsonArray>()) {
    if (diagnosticCount >= kDiagnosticCapacity) break;
    DiagnosticEntry& entry = diagnosticEntries[diagnosticHead];
    entry.uptimeSeconds = item["t"] | 0UL;
    const char* level = item["l"] | "W";
    entry.level = level[0];
    entry.message = String((const char*)(item["m"] | ""));
    diagnosticHead = (diagnosticHead + 1) % kDiagnosticCapacity;
    ++diagnosticCount;
  }
}
String wifiName, wifiPassword;
String youtubeKey, youtubeChannelId;
String adminUser = "admin", adminSalt, adminHash;
bool forcePasswordChange = true;
bool apEnabled = true;
// Temporary recovery access point. Never persisted to NVS.
bool recoveryApActive = false;
uint32_t wifiDisconnectedSince = 0;
constexpr uint32_t kRecoveryApDelayMs = 60000UL;
String deviceHostname = "cyd-youtube";
String apName = "CYD-YOUTUBE";
String apPassword = "creator1234";
uint8_t apChannel = 6;
uint8_t apMaxClients = 4;
bool apHidden = false;
bool apOpen = false;
// ESP32 CYD radio is 2.4 GHz only. AP channel width is managed by the driver;
// 20/40 MHz is intentionally not offered as a guaranteed selectable mode.

// Passwords are PBKDF2-HMAC-SHA256 hashes, not recoverable plaintext.

uint16_t refreshMinutes = 5;
uint32_t dailyQuotaLimit = 10000;
uint16_t quotaReserve = 1000;
uint32_t quotaUsed = 0;
uint32_t quotaDay = 0;  // Days since Unix epoch in America/Los_Angeles.
uint32_t lastQuotaPersist = 0;
uint32_t lastYoutubePoll = 0;
String youtubeStatus = "Not configured";
String channelAvatarUrl;
String cachedAvatarUrl;
// Avatar is downloaded only once per boot, on explicit connection refresh, or channel change.
bool avatarRefreshPending = true;
uint8_t* avatarJpeg = nullptr;
size_t avatarJpegLength = 0;
constexpr size_t kMaxAvatarBytes = 24576;
constexpr int kAvatarX = 12, kAvatarY = 9, kAvatarSize = 30;
int avatarDrawX = 0, avatarDrawY = 0;

// TJpg_Decoder streams decoded RGB565 blocks to the existing TFT_eSPI instance.
bool avatarBlock(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t* pixels) {
  if (x >= kAvatarSize || y >= kAvatarSize) return false;
  display.pushImage(avatarDrawX + x, avatarDrawY + y, w, h, pixels);
  return true;
}

bool dirty = true;  // Display refresh flag; declared before avatar fetch uses it.

// Verify Google TLS peer certificates. No insecure fallback is permitted.
// HTTPS requests wait for NTP so the certificate validity period can be checked.
void configureGoogleTLS(WiFiClientSecure& client) {
  client.setCACert(kGoogleTrustRoots);
}

void fetchAvatar(bool force = false) {
  if (!force && channelAvatarUrl == cachedAvatarUrl && avatarJpeg) return;
  if (!SecurityPolicy::isGoogleImageHost(channelAvatarUrl) || WiFi.status() != WL_CONNECTED || !SecurityPolicy::clockReady()) return;
  WiFiClientSecure client;
  configureGoogleTLS(client);
  HTTPClient request;
  request.setTimeout(8000);
  request.setFollowRedirects(HTTPC_DISABLE_FOLLOW_REDIRECTS);
  if (!request.begin(client, channelAvatarUrl)) return;
  const uint32_t startMs = millis();
  const int status = request.GET();
  const int length = request.getSize();
  uint8_t* replacement = nullptr;
  if (status == HTTP_CODE_OK && length >= 2 && length <= (int)kMaxAvatarBytes) {
    uint8_t* buffer = (uint8_t*)malloc(length);
    if (buffer) {
      const int got = request.getStream().readBytes(buffer, length);
      if (got == length && buffer[0] == 0xFF && buffer[1] == 0xD8) replacement = buffer;
      else free(buffer);
    }
  }
  request.end();
  if (replacement) {
    if (avatarJpeg) free(avatarJpeg);
    avatarJpeg = replacement;
    avatarJpegLength = length;
    cachedAvatarUrl = channelAvatarUrl;
    dirty = true;
  }
  Serial.printf("Avatar HTTPS: %d, %lu ms, %u bytes\n", status,
                (unsigned long)(millis() - startMs), (unsigned)avatarJpegLength);
}

void drawChannelAvatar() {
  display.fillRoundRect(kAvatarX, kAvatarY, kAvatarSize, kAvatarSize, 7, kDarkGreen);
  if (avatarJpeg && avatarJpegLength) {
    uint16_t width = 0, height = 0;
    if (TJpgDec.getJpgSize(&width, &height, avatarJpeg, avatarJpegLength) == JDR_OK && width && height) {
      // Default YouTube thumbnail is normally 88x88. 1/4 scale fits 30px.
      uint8_t scale = 1;
      while (scale < 8 && (width / scale > kAvatarSize || height / scale > kAvatarSize)) scale *= 2;
      TJpgDec.setJpgScale(scale);
      avatarDrawX = kAvatarX + (kAvatarSize - width / scale) / 2;
      avatarDrawY = kAvatarY + (kAvatarSize - height / scale) / 2;
      if (TJpgDec.drawJpg(0, 0, avatarJpeg, avatarJpegLength) == JDR_OK) return;
    }
  }
  // Fallback icon when image is unavailable or cannot be decoded.
  display.fillCircle(27, 24, 8, kWhite);
  display.fillCircle(27, 24, 4, kPurple);
}

String resolvedChannelId;
uint32_t youtubeRequests = 0;
// Persistent daily end-of-day snapshots (public channel totals). History begins at first successful sync.
struct HistoryPoint { uint32_t day; uint64_t subscribers; uint64_t views; };
struct HistoryStore {
  uint32_t magic;
  uint32_t channelHash;
  uint8_t count;
  uint8_t reserved[3];
  HistoryPoint points[32];
};
HistoryStore history = {};
constexpr uint32_t kHistoryMagic = 0x48495339UL;
const uint8_t historyPeriods[] = {1, 2, 3, 7, 14, 30};
uint8_t historyPeriodIndex = 3;
uint64_t currentSubscribers = 0, currentViews = 0;
bool currentTotalsValid = false;
uint32_t lastHistoryWriteDay = 0;
uint64_t lastHistoryWriteSubs = 0, lastHistoryWriteViews = 0;

uint32_t hashChannel(const String& id) {
  uint32_t h = 2166136261UL;
  for (size_t i = 0; i < id.length(); ++i) { h ^= uint8_t(id[i]); h *= 16777619UL; }
  return h;
}

void loadHistory(uint32_t channelHash) {
  prefs.begin("history", true);
  size_t n = prefs.getBytesLength("daily");
  if (n == sizeof(history)) prefs.getBytes("daily", &history, sizeof(history));
  prefs.end();
  if (n != sizeof(history) || history.magic != kHistoryMagic ||
      history.channelHash != channelHash || history.count > 32) {
    memset(&history, 0, sizeof(history));
    history.magic = kHistoryMagic;
    history.channelHash = channelHash;
  }
  lastHistoryWriteDay = 0;
}

void storeHistory(uint64_t subs, uint64_t views) {
  time_t now = time(nullptr);
  if (now < 1700000000 || history.channelHash == 0) return;
  const uint32_t day = uint32_t(now / 86400); // UTC day; documented in UI.
  if (history.count && history.points[history.count - 1].day == day) {
    HistoryPoint& point = history.points[history.count - 1];
    if (point.subscribers == subs && point.views == views) return;
    point.subscribers = subs;
    point.views = views;
  } else {
    if (history.count == 32) {
      memmove(&history.points[0], &history.points[1], 31 * sizeof(HistoryPoint));
      history.count = 31;
    }
    history.points[history.count++] = {day, subs, views};
  }
  // Limit flash wear: once per UTC day, and at most once per hour for updates.
  if (lastHistoryWriteDay != day || millis() - lastQuotaPersist >= 3600000UL ||
      lastHistoryWriteSubs == 0) {
    prefs.begin("history", false);
    prefs.putBytes("daily", &history, sizeof(history));
    prefs.end();
    lastHistoryWriteDay = day;
    lastHistoryWriteSubs = subs;
    lastHistoryWriteViews = views;
  }
}

bool historyDelta(bool subscribers, uint8_t days, int64_t& delta, double& percent, uint8_t& availableDays) {
  if (!currentTotalsValid || !history.count) return false;
  const time_t now = time(nullptr);
  if (now < 1700000000) return false;
  const uint32_t today = uint32_t(now / 86400);
  if (today < days) return false;
  const uint32_t target = today - days;
  // Use the latest baseline on or before the requested start. When history is
  // younger than the selected period, use its earliest snapshot instead and
  // explicitly label the result as partial coverage (never as a full period).
  int match = -1;
  for (int i = 0; i < history.count; ++i) {
    if (history.points[i].day <= target &&
        (match < 0 || history.points[i].day > history.points[match].day)) match = i;
  }
  if (match < 0) {
    for (int i = 0; i < history.count; ++i) {
      if (history.points[i].day <= today &&
          (match < 0 || history.points[i].day < history.points[match].day)) match = i;
    }
  }
  if (match < 0) return false;
  const uint64_t oldValue = subscribers ? history.points[match].subscribers : history.points[match].views;
  const uint64_t current = subscribers ? currentSubscribers : currentViews;
  delta = current >= oldValue ? int64_t(current - oldValue) : -int64_t(oldValue - current);
  percent = oldValue ? double(delta) * 100.0 / double(oldValue) : 0.0;
  availableDays = uint8_t(today - history.points[match].day);
  return true;
}


uint8_t brightness = 190;
uint8_t rotation = 1;
bool panelInverted = true;
bool snapshotInverted = false; // Web-only TFT readback color inversion; independent of panel inversion.
uint8_t snapshotColorMode = 1; // Default: byte-swapped RGB565 (verified on reference CYD); user-selectable 0..3
wl_status_t previousWifiStatus = WL_IDLE_STATUS;
uint32_t colorTestUntil = 0;
uint8_t page = 0; // 0 home, 1 subscribers, 2 views, 3 best, 4 latest, 5 likes, 6 comments
uint32_t lastPoll = 0, lastFrame = 0, lastTouch = 0;
bool firstPollPending = true;
uint32_t screenRevision = 0;
void processScreenTouch(int x, int y);
uint32_t lastSuccessfulStatisticsMs = 0;
uint32_t lastWifiAttempt = 0;

int16_t rawMinX = 200, rawMaxX = 3800, rawMinY = 200, rawMaxY = 3800;

uint16_t dimColor(uint16_t color, uint8_t factor) {
  uint8_t r = ((color >> 11) & 31) * factor / 255;
  uint8_t g = ((color >> 5) & 63) * factor / 255;
  uint8_t b = (color & 31) * factor / 255;
  return (r << 11) | (g << 5) | b;
}

void eye(int x, int y, int size) {
  const int h = size / 2;
  display.fillTriangle(x, y + h - 2, x + h, y, x + size, y + h - 2, kDarkGreen);
  display.fillTriangle(x, y + h + 2, x + h, y + size, x + size, y + h + 2, kGreen);
  display.fillCircle(x + h, y + h, size / 4, kWhite);
  display.fillCircle(x + h, y + h + 1, size / 7, kPurple);
  display.fillCircle(x + h + 1, y + h + 2, size / 11, TFT_BLACK);
  display.fillCircle(x + h - 3, y + h - 3, max(1, size / 22), kWhite);
  display.fillRect(x, y + h - 2, size, 3, kBackground);
}

// UI v2: fixed 320x240 layout, safe text clipping, no overlapping labels.
void uiText(const String& value, int x, int y, int font, uint16_t color,
            uint16_t background = kBackground, uint8_t datum = TL_DATUM) {
  display.setTextDatum(datum);
  display.setTextColor(color, background);
  display.drawString(value, x, y, font);
}

String clipped(String value, size_t maxChars) {
  if (value.length() <= maxChars) return value;
  if (maxChars <= 3) return value.substring(0, maxChars);
  return value.substring(0, maxChars - 3) + "...";
}

String compactNumber(const String& value) {
  // Only abbreviate pure numeric counters; titles remain text.
  if (!value.length()) return "--";
  for (size_t i = 0; i < value.length(); ++i)
    if (value[i] < '0' || value[i] > '9') return value;
  const double n = value.toDouble();
  if (n >= 1000000.0) return String(n / 1000000.0, n >= 10000000.0 ? 0 : 1) + "M";
  if (n >= 10000.0) return String(n / 1000.0, n >= 100000.0 ? 0 : 1) + "K";
  return value;
}

// Pixel-aware ellipsis: never draw text outside the allotted rectangle.
String fitText(String text, int font, int maxWidth) {
  if (display.textWidth(text, font) <= maxWidth) return text;
  while (text.length() && display.textWidth(text + "...", font) > maxWidth)
    text.remove(text.length() - 1);
  return text + "...";
}

void statusHeader() {
  display.fillRect(0, 0, 320, 56, kBackground);
  drawChannelAvatar();
  const String title = data.channel;
  // Two lines for longer channel names; reserve right side for Wi-Fi status.
  if (display.textWidth(title, 2) <= 235) {
    uiText(title, 51, 9, 2, kWhite);
  } else {
    int split = 0;
    for (int i = 1; i <= (int)title.length(); ++i) {
      if (display.textWidth(title.substring(0, i), 1) > 232) break;
      split = i;
    }
    if (split < 1) split = 1;
    int word = title.lastIndexOf(' ', split);
    if (word > 8) split = word;
    String first = title.substring(0, split);
    String second = title.substring(split);
    second.trim();
    uiText(fitText(first, 1, 232), 51, 9, 1, kWhite);
    uiText(fitText(second, 1, 232), 51, 20, 1, kWhite);
  }
  const bool online = WiFi.status() == WL_CONNECTED;
  const String ip = online ? WiFi.localIP().toString() : WiFi.softAPIP().toString();
  uiText(ip, 52, 39, 1, kMuted);
  display.fillCircle(302, 17, 4, online ? kGreen : kPurple);
  display.drawFastHLine(12, 55, 296, kLine);
}

void tile(int x, int y, int w, int h, const Metric& m, uint16_t accent, bool numeric) {
  display.fillRoundRect(x, y, w, h, 9, kCard);
  display.fillRoundRect(x + 9, y + 9, 3, 11, 1, accent);
  uiText(fitText(m.label, 1, w - 33), x + 18, y + 10, 1, kMuted, kCard);
  if (m.unread > 0) display.fillCircle(x + w - 10, y + 14, 2, accent);
  if (numeric) {
    String value = compactNumber(m.value);
    int font = display.textWidth(value, 4) <= w - 18 ? 4 : 2;
    uiText(fitText(value, font, w - 18), x + 9, y + 28, font, kWhite, kCard);
  } else {
    // Title stays on a single line; reserve the caption below it.
    uiText(fitText(m.value, 1, w - 18), x + 9, y + 33, 1, kWhite, kCard);
  }
  uiText(fitText(m.change, 1, w - 18), x + 9, y + h - 14, 1, accent, kCard);
}

void statusFooter() {
  display.fillRect(0, 221, 320, 19, kBackground);
  display.drawFastHLine(12, 219, 296, kLine);
  uiText(data.live ? "LIVE DATA" : clipped(youtubeStatus, 29), 12, 226, 1,
         data.live ? kGreen : kPurple);
  uiText(String(refreshMinutes) + "m", 286, 226, 1, kMuted);
}

void home(uint8_t phase) {
  (void)phase;
  statusHeader();
  tile(12, 63, 143, 70, data.subscribers, kGreen, true);
  tile(165, 63, 143, 70, data.views, kPurple, true);
  tile(12, 143, 143, 69, data.best, kGreen, false);
  tile(165, 143, 143, 69, data.latest, kPurple, false);
  statusFooter();
}

Metric& metricForPage(uint8_t p) {
  switch (p) {
    case 1: return data.subscribers;
    case 2: return data.views;
    case 3: return data.best;
    case 4: return data.latest;
    case 5: return data.likes;
    default: return data.comments;
  }
}

// Draw up to maxLines of text with width-aware wrapping. Never draw beyond card bounds.
void wrappedText(const String& input, int x, int y, int maxWidth, int maxLines,
                 int font, uint16_t color, uint16_t bg, int lineHeight) {
  String remaining = input;
  remaining.replace("\n", " ");
  remaining.trim();
  for (int line = 0; line < maxLines && remaining.length(); ++line) {
    int fit = 0;
    for (int n = 1; n <= (int)remaining.length(); ++n) {
      if (display.textWidth(remaining.substring(0, n), font) > maxWidth) break;
      fit = n;
    }
    if (fit <= 0) break;
    if (fit == (int)remaining.length()) {
      uiText(remaining, x, y + line * lineHeight, font, color, bg);
      return;
    }
    int cut = fit;
    while (cut > 0 && remaining[cut - 1] != ' ') --cut;
    if (cut < fit / 2) cut = fit;
    String part = remaining.substring(0, cut);
    part.trim();
    remaining = remaining.substring(cut);
    remaining.trim();
    if (line == maxLines - 1 && remaining.length()) {
      while (part.length() && display.textWidth(part + "...", font) > maxWidth)
        part.remove(part.length() - 1);
      part += "...";
    }
    uiText(part, x, y + line * lineHeight, font, color, bg);
  }
}

void detail(uint8_t phase) {
  (void)phase;
  Metric& m = metricForPage(page);
  statusHeader();
  display.fillRoundRect(12, 62, 296, 151, 10, kCard);
  display.fillRoundRect(20, 69, 53, 21, 5, kDarkGreen);
  uiText("< BACK", 26, 76, 1, kWhite, kDarkGreen);
  uiText(fitText(m.label, 2, 210), 84, 71, 2, kMuted, kCard);
  display.drawFastHLine(20, 95, 280, kLine);
  const bool video = page == 3 || page == 4;
  if (video) {
    const VideoStats& v = page == 3 ? topVideo : latestVideo;
    wrappedText(v.valid ? v.title : m.value, 21, 101, 277, 2, 1, kWhite, kCard, 12);
    display.drawFastHLine(21, 127, 278, kLine);
    if (v.valid) {
      uiText("VIEWS", 22, 135, 1, kMuted, kCard);
      uiText(fitText(compactNumber(v.views), 2, 84), 22, 147, 2, kWhite, kCard);
      uiText("LIKES", 121, 135, 1, kMuted, kCard);
      uiText(fitText(compactNumber(v.likes), 2, 77), 121, 147, 2, kWhite, kCard);
      uiText("COMMENTS", 220, 135, 1, kMuted, kCard);
      uiText(fitText(compactNumber(v.comments), 2, 74), 220, 147, 2, kWhite, kCard);
      display.drawFastHLine(21, 173, 278, kLine);
      uiText("PUBLISHED", 22, 181, 1, kMuted, kCard);
      uiText(fitText(v.published, 1, 94), 22, 194, 1, kGreen, kCard);
      uiText("DURATION", 158, 181, 1, kMuted, kCard);
      uiText(fitText(v.duration, 1, 124), 158, 194, 1, kGreen, kCard);
    } else {
      uiText("Video statistics unavailable", 22, 145, 1, kPurple, kCard);
    }
  } else if (page == 1 || page == 2) {
    // Historical change view: choose period directly with left/right touch arrows.
    const uint8_t days = historyPeriods[historyPeriodIndex];
    const bool subs = page == 1;
    const uint64_t current = subs ? currentSubscribers : currentViews;
    uiText(fitText(currentTotalsValid ? String((unsigned long)current) : "--", 4, 260),
           23, 103, 4, kWhite, kCard);
    display.drawFastHLine(22, 141, 274, kLine);
    display.fillRoundRect(22, 148, 40, 23, 5, kDarkGreen);
    display.fillRoundRect(248, 148, 40, 23, 5, kDarkGreen);
    uiText("<", 39, 153, 2, kWhite, kDarkGreen);
    uiText(">", 265, 153, 2, kWhite, kDarkGreen);
    uiText(String(days) + (days == 1 ? " DAY" : " DAYS"), 120, 153, 2, kWhite, kCard);
    int64_t change = 0;
    double percentage = 0;
    uint8_t actualDays = 0;
    if (historyDelta(subs, days, change, percentage, actualDays)) {
      String sign = change > 0 ? "+" : "";
      String amount = sign + String((long)change);
      String pct = sign + String(percentage, 1) + "%";
      uiText(fitText(amount + "  (" + pct + ")", 2, 272), 23, 180, 2,
             change >= 0 ? kGreen : kPurple, kCard);
      if (actualDays < days) uiText("Partial history: " + String(actualDays) + "d of " + String(days) + "d", 23, 200, 1, kMuted, kCard);
      else if (actualDays > days) uiText("Baseline " + String(actualDays) + "d old", 23, 200, 1, kMuted, kCard);
    } else {
      uiText("Collecting history...", 23, 184, 2, kMuted, kCard);
      uiText("Daily snapshots begin at first sync", 23, 201, 1, kMuted, kCard);
    }
  } else {
    const String number = m.value;
    uiText(fitText(number, number.length() <= 8 ? 4 : 2, 268), 24, 108,
           number.length() <= 8 ? 4 : 2, kWhite, kCard);
    wrappedText(m.change, 24, 157, 270, 2, 2, kGreen, kCard, 18);
  }
  statusFooter();
}

void render() {
  display.fillScreen(kBackground);
  if (page == 0) home(0);
  else detail(0);
  dirty = false;
  ++screenRevision;
}

// Only outlines are updated during animation; no full-screen repaint.
void pulseOutline(int x, int y, int w, int h, uint16_t accent, uint8_t phase) {
  const uint8_t wave = phase < 128 ? phase : 255 - phase;
  const uint8_t intensity = 40 + wave / 2;
  for (int i = 1; i <= 3; ++i)
    display.drawRoundRect(x - i, y - i, w + 2 * i, h + 2 * i, 10,
                          dimColor(accent, intensity / (i + 1)));
}

void animateUnread() {
  if (page != 0) return;
  const uint8_t phase = (millis() / 10) & 255;
  if (data.subscribers.unread) pulseOutline(12, 63, 143, 70, kGreen, phase);
  if (data.views.unread) pulseOutline(165, 63, 143, 70, kPurple, phase);
  if (data.best.unread) pulseOutline(12, 143, 143, 69, kGreen, phase);
  if (data.latest.unread) pulseOutline(165, 143, 143, 69, kPurple, phase);
}

void showColorBars() {
  const uint16_t colors[] = {TFT_RED, TFT_GREEN, TFT_BLUE, TFT_WHITE, TFT_BLACK};
  const int width = display.width() / 5;
  for (int i = 0; i < 5; ++i) {
    const int x = i * width;
    display.fillRect(x, 0, i == 4 ? display.width() - x : width, display.height(), colors[i]);
  }
}

// RFC 8018 PBKDF2-HMAC-SHA256, one 32-byte block, 12000 iterations.
String toHex(const uint8_t* bytes, size_t length) {
  const char* digits="0123456789abcdef"; String out; out.reserve(length*2);
  for(size_t i=0;i<length;++i){out+=digits[bytes[i]>>4];out+=digits[bytes[i]&15];}
  return out;
}
String passwordDigest(const String& password, const String& saltHex) {
  if(saltHex.length()!=32) return "";
  uint8_t salt[20];
  for(int i=0;i<16;++i) {
    char hi=saltHex[i*2], lo=saltHex[i*2+1];
    auto digit=[](char c)->int {if(c>='0'&&c<='9')return c-'0';if(c>='a'&&c<='f')return c-'a'+10;return -1;};
    int a=digit(hi),b=digit(lo); if(a<0||b<0)return "";
    salt[i]=uint8_t(a*16+b);
  }
  salt[16]=0;salt[17]=0;salt[18]=0;salt[19]=1;
  uint8_t u[32], result[32]={0};
  const mbedtls_md_info_t* md=mbedtls_md_info_from_type(MBEDTLS_MD_SHA256);
  if(!md || mbedtls_md_hmac(md,(const unsigned char*)password.c_str(),password.length(),salt,20,u)!=0)return "";
  memcpy(result,u,32);
  for(int i=1;i<12000;++i) {
    if(mbedtls_md_hmac(md,(const unsigned char*)password.c_str(),password.length(),u,32,u)!=0)return "";
    for(int j=0;j<32;++j)result[j]^=u[j];
  }
  return toHex(result,32);
}
void setAdminPassword(const String& password, bool mustChange) {
  uint8_t salt[16];esp_fill_random(salt,sizeof(salt));
  adminSalt=toHex(salt,sizeof(salt));adminHash=passwordDigest(password,adminSalt);
  forcePasswordChange=mustChange;
  prefs.begin("creator",false);
  prefs.putString("admuser",adminUser);prefs.putString("admsalt",adminSalt);
  prefs.putString("admhash",adminHash);prefs.putBool("forcepw",forcePasswordChange);
  prefs.remove("adminpw");prefs.end();
}
void migrateLegacyPassword(const String& password) {setAdminPassword(password,true);}
bool securePasswordEqual(const String& a,const String& b) {
  if(a.length()!=64||b.length()!=64)return false;
  uint8_t diff=0;for(int i=0;i<64;++i)diff|=uint8_t(a[i]^b[i]);return diff==0;
}
void applyApMode() {
  if(apEnabled || recoveryApActive) {
    WiFi.mode(WIFI_AP_STA);
    WiFi.softAP(apName.c_str(),apOpen ? nullptr : apPassword.c_str(),apChannel,apHidden,apMaxClients);
  }
  else {WiFi.softAPdisconnect(true);WiFi.mode(WIFI_STA);}
}
void saveSettings() {
  prefs.begin("creator", false);
  prefs.putString("hostname",deviceHostname);
  prefs.putString("apname",apName);
  prefs.putString("appass",apPassword);
  prefs.putUChar("apchannel",apChannel);
  prefs.putUChar("apmax",apMaxClients);
  prefs.putBool("aphidden",apHidden);
  prefs.putBool("apopen",apOpen);
  prefs.putString("ssid", wifiName);
  prefs.putString("password", wifiPassword);
  prefs.putString("ytkey", youtubeKey);
  prefs.putString("ytchan", youtubeChannelId);
  prefs.remove("adminpw"); // Delete legacy plaintext password after migration.
  prefs.putUShort("refresh", refreshMinutes);
  prefs.putUInt("qlimit", dailyQuotaLimit);
  prefs.putUShort("qreserve", quotaReserve);
  prefs.putUChar("bright", brightness);
  prefs.putUChar("rotation", rotation);
  prefs.putBool("invert", panelInverted);
  prefs.putBool("snapinvert", snapshotInverted);
  prefs.putUChar("snapmode", snapshotColorMode);
  prefs.putBool("ledenable",ledEnabled);
  prefs.putUChar("ledbright",ledBrightness);
  prefs.putUShort("leddur",ledDurationSeconds);
  for(int i=0;i<8;++i) prefs.putUShort(kThemeKeys[i],*kThemeValues[i]);
  for(int i=0;i<5;++i) prefs.putUInt(kWebKeys[i],webColors[i]);
  for(int i=0;i<EVENT_COUNT;++i) {
    prefs.putUInt((String(kEventKeys[i])+"color").c_str(),eventColors[i]);
    prefs.putBool((String(kEventKeys[i])+"enabled").c_str(),eventEnabled[i]);
  }
  prefs.end();
}

void loadSettings() {
  prefs.begin("creator", true);
  deviceHostname = prefs.getString("hostname", "cyd-youtube");
  apName = prefs.getString("apname", "CYD-YOUTUBE");
  apPassword = prefs.getString("appass", "creator1234");
  apChannel = prefs.getUChar("apchannel", 6);
  apMaxClients = prefs.getUChar("apmax", 4);
  apHidden = prefs.getBool("aphidden",false);
  apOpen = prefs.getBool("apopen",false);
  if(apChannel<1||apChannel>11)apChannel=6;
  if(apMaxClients<1||apMaxClients>8)apMaxClients=4;
  wifiName = prefs.getString("ssid", "");
  wifiPassword = prefs.getString("password", "");
  youtubeKey = prefs.getString("ytkey", "");
  youtubeChannelId = prefs.getString("ytchan", "");
  String legacyPassword = prefs.isKey("adminpw") ? prefs.getString("adminpw", "") : "";
  adminUser = prefs.getString("admuser", "admin");
  adminSalt = prefs.getString("admsalt", "");
  adminHash = prefs.getString("admhash", "");
  forcePasswordChange = prefs.getBool("forcepw", true);
  apEnabled = prefs.getBool("apenable", true);
  refreshMinutes = prefs.getUShort("refresh", 5);
  if (refreshMinutes < 1 || refreshMinutes > 1440) refreshMinutes = 5;
  dailyQuotaLimit = prefs.getUInt("qlimit", 10000);
  if (dailyQuotaLimit < 1) dailyQuotaLimit = 10000;
  quotaReserve = prefs.getUShort("qreserve", 1000);
  quotaUsed = prefs.getUInt("qused", 0);
  quotaDay = prefs.getUInt("qday", 0);
  brightness = prefs.getUChar("bright", 190);
  rotation = prefs.getUChar("rotation", 1);
  panelInverted = prefs.getBool("invert", true);
  snapshotInverted = prefs.getBool("snapinvert", false);
  snapshotColorMode = prefs.getUChar("snapmode", 1);
  if (snapshotColorMode > 3) snapshotColorMode = 1;
  ledEnabled = prefs.getBool("ledenable",true);
  ledBrightness = prefs.getUChar("ledbright",90);
  ledDurationSeconds = prefs.getUShort("leddur",12);
  ledDurationSeconds = constrain(ledDurationSeconds,1,120);
  for(int i=0;i<8;++i) *kThemeValues[i]=prefs.getUShort(kThemeKeys[i],*kThemeValues[i]);
  for(int i=0;i<5;++i) webColors[i]=prefs.getUInt(kWebKeys[i],webColors[i]);
  for(int i=0;i<EVENT_COUNT;++i) {
    eventColors[i]=prefs.getUInt((String(kEventKeys[i])+"color").c_str(),eventColors[i]);
    eventEnabled[i]=prefs.getBool((String(kEventKeys[i])+"enabled").c_str(),eventEnabled[i]);
  }
  prefs.end();
  // Upgrade an existing installation without resetting its credentials.
  if (adminHash.isEmpty() && !legacyPassword.isEmpty()) {
    // Migrated below after the hashing helper is declared.
    migrateLegacyPassword(legacyPassword);
  }
}

void applyBrightness() {
  writeBacklightPwm(brightness);
}

String escapeHtml(const String& input) {
  String out;
  out.reserve(input.length() + 12);
  for (size_t i = 0; i < input.length(); ++i) {
    switch (input[i]) {
      case '&': out += "&amp;"; break;
      case '<': out += "&lt;"; break;
      case '>': out += "&gt;"; break;
      case '"': out += "&quot;"; break;
      case '\'': out += "&#39;"; break;
      default: out += input[i];
    }
  }
  return out;
}

String wifiStatusText() {
  if (WiFi.status() == WL_CONNECTED) {
    return "Connected to " + escapeHtml(WiFi.SSID()) + " | IP " + WiFi.localIP().toString();
  }
  return wifiName.isEmpty() ? "Not configured" : "Disconnected / reconnecting";
}


// All YouTube API traffic is sent directly by the ESP32. No PC or bridge is required.
// Google API transport supports fast compatibility and strict CA verification modes.
uint32_t pacificDay() {
  time_t now = time(nullptr);
  if (now < 1700000000) return 0; // No trustworthy time yet; don't reset the quota.
  struct tm t;
  localtime_r(&now, &t);
  return uint32_t(t.tm_year + 1900) * 1000 + uint32_t(t.tm_yday + 1);
}

void persistQuota() {
  prefs.begin("creator", false);
  prefs.putUInt("qused", quotaUsed);
  prefs.putUInt("qday", quotaDay);
  prefs.end();
  lastQuotaPersist = millis();
}

void rolloverQuota() {
  uint32_t today = pacificDay();
  if (!today) return;
  if (!quotaDay) { quotaDay = today; persistQuota(); }
  else if (today != quotaDay) {
    quotaDay = today;
    quotaUsed = 0;
    persistQuota();
  }
}

bool quotaAllows(uint16_t cost) {
  rolloverQuota();
  return quotaUsed + cost <= (dailyQuotaLimit > quotaReserve ? dailyQuotaLimit - quotaReserve : 0);
}

// HTTP request accounting is performed BEFORE sending: failed requests can consume quota.
// YouTube Data API list methods used here each have a documented base cost of 1 unit.
bool youtubeGet(const String& path, DynamicJsonDocument& json) {
  if (WiFi.status() != WL_CONNECTED) { youtubeStatus = "Wi-Fi disconnected"; return false; }
  if (youtubeKey.isEmpty()) { youtubeStatus = "API key missing"; return false; }
  if (!quotaAllows(1)) { youtubeStatus = "Quota reserve reached"; return false; }
  if (!SecurityPolicy::clockReady()) { youtubeStatus = "Waiting for NTP time (TLS)"; return false; }
  ++quotaUsed;
  ++youtubeRequests;
  persistQuota();
  WiFiClientSecure tls;
  configureGoogleTLS(tls); // Certificate verification required.
  HTTPClient http;
  http.setTimeout(12000);
  http.useHTTP10(true); // Avoid chunked streaming incompatibilities with ArduinoJson.
  http.setReuse(false); // Explicit Connection: close, no keep-alive reuse.
  String url = "https://www.googleapis.com/youtube/v3/" + path + (path.indexOf('?') >= 0 ? "&" : "?") + "key=" + youtubeKey;
  if (!http.begin(tls, url)) { youtubeStatus = "HTTPS setup failed"; return false; }
  const uint32_t requestStartMs = millis();
  int code = http.GET();
  bool ok = false;
  Serial.printf("YouTube HTTP status: %d, heap: %u\n", code, (unsigned)ESP.getFreeHeap());
  if (code == HTTP_CODE_OK) {
    // Buffer the response before parsing: TLS streams can return partial/chunked data.
    String payload = http.getString();
    DeserializationError err = deserializeJson(json, payload);
    ok = !err;
    if (ok) youtubeStatus = "Live";
    else {
      youtubeStatus = "JSON: " + String(err.c_str()) + " (" + String(payload.length()) + " bytes)";
      Serial.printf("YouTube JSON error: %s, payload bytes: %u\n", err.c_str(), (unsigned)payload.length());
      // Do not print API response: it can contain private configuration details.
    }
  } else {
    youtubeStatus = "YouTube HTTP " + String(code);
  }
  http.end();
  // HTTPClient::end() already closes the client. Avoid a redundant TLS stop.
  // mbedTLS -0x004C can still be logged by the core on peer shutdown.
  if (youtubeStatus != "Live") recordDiagnostic('W', "YouTube request: " + youtubeStatus);
  else debugDiagnostic("YouTube request succeeded");
  Serial.printf("YouTube API: %s, %lu ms\n", youtubeStatus.c_str(), (unsigned long)(millis() - requestStartMs));
  return ok;
}

String compactNumber(uint64_t n) {
  if (n >= 1000000ULL) return String(double(n) / 1000000.0, 1) + "M";
  if (n >= 10000ULL) return String(double(n) / 1000.0, 1) + "K";
  return String((unsigned long)n);
}

void pollYouTube() {
  data.live = false;
  // Retain last valid video cards if a partial request fails.
  youtubeStatus = "Fetching YouTube";
  dirty = true;
  if (youtubeKey.isEmpty() || youtubeChannelId.isEmpty()) {
    youtubeStatus = "Enter API key and Channel ID";
    data.updated = youtubeStatus;
    return;
  }
  String selector = youtubeChannelId;
  selector.trim();
  if (selector.startsWith("https://www.youtube.com/@")) selector = selector.substring(25);
  else if (selector.startsWith("https://youtube.com/@")) selector = selector.substring(21);
  // channels.list supports forHandle=@name, not just UC... identifiers.
  String channelQuery = selector.startsWith("UC") ? "id=" + selector :
      "forHandle=" + (selector.startsWith("@") ? selector : "@" + selector);
  DynamicJsonDocument channel(8192);
  if (!youtubeGet("channels?part=snippet,statistics,contentDetails&fields=items(id,snippet(title,thumbnails),statistics(subscriberCount,viewCount),contentDetails(relatedPlaylists(uploads)))&" + channelQuery, channel)) { data.updated = youtubeStatus; return; }
  JsonArrayConst items = channel["items"].as<JsonArrayConst>();
  if (items.size() == 0) { youtubeStatus = "Channel not found (check handle/UC ID)"; data.updated = youtubeStatus; return; }
  JsonObjectConst ch = items[0];
  data.channel = ch["snippet"]["title"] | "YouTube Channel";
  resolvedChannelId = ch["id"] | "";
  channelAvatarUrl = ch["snippet"]["thumbnails"]["medium"]["url"] | "";
  if (channelAvatarUrl.isEmpty()) channelAvatarUrl = ch["snippet"]["thumbnails"]["default"]["url"] | "";
  // Never fetch the avatar during periodic statistics polling.
  // Refresh once at boot, when the channel changes, or on explicit connection refresh.
  uint64_t subs = strtoull(ch["statistics"]["subscriberCount"] | "0", nullptr, 10);
  uint64_t views = strtoull(ch["statistics"]["viewCount"] | "0", nullptr, 10);
  const uint32_t channelIdentity = hashChannel(resolvedChannelId);
  if (history.channelHash != channelIdentity) loadHistory(channelIdentity);
  if (prevChannelIdentity != resolvedChannelId) {
    havePrevCounters = false;
    prevChannelIdentity = resolvedChannelId;
  }
  if(havePrevCounters) {
    if(subs>prevSubs) notifyEvent(SUB_EVENT);
    else if(views>prevViews) notifyEvent(VIEW_EVENT);
  }
  prevSubs=subs; prevViews=views;
  currentSubscribers = subs;
  currentViews = views;
  currentTotalsValid = true;
  storeHistory(subs, views);
  data.subscribers.value = compactNumber(subs);
  data.subscribers.change = "Public count (rounded)";
  data.views.value = compactNumber(views);
  data.views.change = "Lifetime channel views";
  String uploads = ch["contentDetails"]["relatedPlaylists"]["uploads"] | "";
  bool allDataFetched = false;
  if (uploads.length()) {
    DynamicJsonDocument playlist(8192);
    if (youtubeGet("playlistItems?part=snippet&fields=items(snippet(title,resourceId(videoId)))&maxResults=10&playlistId=" + uploads, playlist)) {
      JsonArrayConst videos = playlist["items"].as<JsonArrayConst>();
      String ids;
      String latestTitle;
      String firstVideoId;
      for (JsonObjectConst item : videos) {
        String id = item["snippet"]["resourceId"]["videoId"] | "";
        if (id.isEmpty()) continue;
        if (ids.length()) ids += ",";
        ids += id;
        if (latestTitle.isEmpty()) { latestTitle = item["snippet"]["title"] | ""; firstVideoId = id; }
      }
      if (latestTitle.length()) { data.latest.value = latestTitle; data.latest.change = "Latest upload"; }
      if (ids.length()) {
        DynamicJsonDocument details(12288);
        if (youtubeGet("videos?part=snippet,statistics,contentDetails&fields=items(id,snippet(title,publishedAt),statistics(viewCount,likeCount,commentCount),contentDetails(duration))&id=" + ids, details)) {
          uint64_t bestViews = 0, totalLikes = 0, totalComments = 0;
          String bestTitle;
          latestVideo = VideoStats(); topVideo = VideoStats();
          for (JsonObjectConst v : details["items"].as<JsonArrayConst>()) {
            uint64_t vv = strtoull(v["statistics"]["viewCount"] | "0", nullptr, 10);
            totalLikes += strtoull(v["statistics"]["likeCount"] | "0", nullptr, 10);
            totalComments += strtoull(v["statistics"]["commentCount"] | "0", nullptr, 10);
            VideoStats current;
            current.id = v["id"] | "";
            current.title = v["snippet"]["title"] | "";
            current.views = vv;
            current.likes = strtoull(v["statistics"]["likeCount"] | "0", nullptr, 10);
            current.comments = strtoull(v["statistics"]["commentCount"] | "0", nullptr, 10);
            current.published = String(v["snippet"]["publishedAt"] | "").substring(0, 10);
            current.duration = v["contentDetails"]["duration"] | "--";
            current.valid = true;
            if (current.id == firstVideoId) latestVideo = current;
            if (vv >= bestViews) { bestViews = vv; bestTitle = current.title; topVideo = current; }
          }
          if(havePrevCounters) {
            if(totalComments>prevComments) notifyEvent(COMMENT_EVENT);
            else if(totalLikes>prevLikes) notifyEvent(LIKE_EVENT);
          }
          prevLikes=totalLikes; prevComments=totalComments;
          allDataFetched = true;
          data.best.value = bestTitle;
          data.best.change = compactNumber(bestViews) + " views (last 10)";
          data.likes.value = compactNumber(totalLikes);
          data.likes.change = "Last 10 uploads";
          data.comments.value = compactNumber(totalComments);
          data.comments.change = "Last 10 uploads";
        }
      }
    }
  }
  data.subscribers.unread = 0;
  data.views.unread = 0;
  data.best.unread = 0;
  data.latest.unread = 0;
  data.likes.unread = 0;
  data.comments.unread = 0;
  havePrevCounters = allDataFetched;
  data.live = true; // Channel counters are valid even if video metadata fails.
  if (allDataFetched) youtubeStatus = "Live";
  else if (youtubeStatus == "Live") youtubeStatus = "Channel live; video details unavailable";
  data.updated = "YouTube direct | " + String(refreshMinutes) + "m";
  lastSuccessfulStatisticsMs = millis();
  dirty = true;
  // Avatar is fetched only on boot/manual connection refresh/channel change,
  // AFTER statistics, so a slow image server never delays the first counters.
  if (avatarRefreshPending) {
    avatarRefreshPending = false;
    fetchAvatar(true);
  }
}

String base64Decode(const String& encoded) {
  unsigned char out[256];size_t written=0;
  if(encoded.length()>320 || mbedtls_base64_decode(out,sizeof(out)-1,&written,(const unsigned char*)encoded.c_str(),encoded.length())!=0)return "";
  out[written]=0;return String((char*)out);
}
bool requireAdmin(bool allowPasswordChange=false) {
  if (web.method() == HTTP_POST) {
    const String origin = web.header("Origin");
    const String host = web.hostHeader();
    if (!SecurityPolicy::sameOrigin(origin, host)) {
      web.send(403, "text/plain", "Cross-origin or missing Origin denied");
      return false;
    }
  }
  web.sendHeader("Cache-Control", "no-store, max-age=0");
  web.sendHeader("X-Content-Type-Options", "nosniff");
  web.sendHeader("X-Frame-Options", "DENY");
  // Factory Basic authentication is deliberately allowed only until password change.
  bool ok = false;
  if(adminHash.isEmpty()) ok=web.authenticate("admin","admin");
  else {
    // WebServer authenticate() compares plaintext; use its Authorization header
    // only to extract credentials, then compare a PBKDF2 hash.
    if(web.hasHeader("Authorization")) {
      String auth=web.header("Authorization");
      if(auth.startsWith("Basic ")) {
        String raw=base64Decode(auth.substring(6));
        int colon=raw.indexOf(':');
        if(colon>0 && raw.substring(0,colon)==adminUser)
          ok=securePasswordEqual(passwordDigest(raw.substring(colon+1),adminSalt),adminHash);
      }
    }
  }
  if(!ok){web.requestAuthentication();return false;}
  if(forcePasswordChange && !allowPasswordChange) {
    web.sendHeader("Location","/account",true);web.send(303,"text/plain","Password change required");return false;
  }
  return true;
}

uint32_t projectedQuota() {
  // Three 1-unit list methods per poll (channel, playlist, videos).
  return uint32_t((1440UL + refreshMinutes - 1) / refreshMinutes) * 3UL;
}

String quotaPanel() {
  uint32_t estimated = projectedQuota();
  // Use explicit 64-bit arithmetic: avoid mixed unsigned types and overflow.
  const uint64_t denominator = dailyQuotaLimit ? dailyQuotaLimit : 1;
  const uint32_t usedPct = static_cast<uint32_t>(
      ((static_cast<uint64_t>(quotaUsed) * 100ULL) / denominator > 100ULL)
          ? 100ULL : (static_cast<uint64_t>(quotaUsed) * 100ULL) / denominator);
  const uint32_t projectedPct = static_cast<uint32_t>(
      ((static_cast<uint64_t>(estimated) * 100ULL) / denominator > 100ULL)
          ? 100ULL : (static_cast<uint64_t>(estimated) * 100ULL) / denominator);
  String h = "<section><h2>YouTube quota (local tracking)</h2>";
  h += "<p>Locally accounted: <b>" + String(quotaUsed) + " / " + String(dailyQuotaLimit) + " units</b></p>";
  h += "<div class='bar'><div class='fill' style='width:" + String(usedPct) + "%'></div></div>";
  h += "<p>Projected full-day: <b>" + String(estimated) + " units (" + String(projectedPct) + "%)</b></p>";
  h += "<div class='bar'><div class='fill forecast' style='width:" + String(projectedPct) + "%'></div></div>";
  h += "<p>Estimated remaining: " + String(dailyQuotaLimit > quotaUsed ? dailyQuotaLimit - quotaUsed : 0) + " units</p>";
  h += "<p>Requests since boot: " + String(youtubeRequests) + " | API status: " + escapeHtml(youtubeStatus) + "</p>";
  h += "<small>Not official Google Cloud usage. Other apps and API errors may affect project quota. Day resets at Pacific midnight after time synchronization.</small></section>";
  return h;
}

// Portable backup schema v1. Intentionally excludes credentials, Wi-Fi SSID,
// quota counters, device identity, cached data and historical measurements.
void sendSafeBackup() {
  DynamicJsonDocument doc(3072);
  doc["schema"] = "cyd-youtube-settings";
  doc["version"] = 1;
  doc["refreshMinutes"] = refreshMinutes;
  doc["quotaLimit"] = dailyQuotaLimit;
  doc["quotaReserve"] = quotaReserve;
  doc["brightness"] = brightness;
  doc["rotation"] = rotation;
  doc["inverted"] = panelInverted;
  doc["ledEnabled"] = ledEnabled;
  doc["ledBrightness"] = ledBrightness;
  doc["ledDurationSeconds"] = ledDurationSeconds;
  JsonObject webTheme = doc.createNestedObject("webTheme");
  for (int i=0;i<5;++i) webTheme[kWebKeys[i]]=hexColor(webColors[i]);
  JsonObject displayTheme = doc.createNestedObject("displayTheme");
  for (int i=0;i<8;++i) displayTheme[kThemeKeys[i]]=hexColor(from565(*kThemeValues[i]));
  JsonObject alerts = doc.createNestedObject("alerts");
  for (int i=0;i<EVENT_COUNT;++i) {
    JsonObject e = alerts.createNestedObject(kEventKeys[i]);
    e["enabled"] = eventEnabled[i];
    e["color"] = hexColor(eventColors[i]);
  }
  String json; serializeJsonPretty(doc,json);
  web.sendHeader("Content-Disposition", "attachment; filename=cyd-youtube-settings.json");
  web.sendHeader("Cache-Control", "no-store");
  web.send(200,"application/json",json);
}

bool restoreSafeBackup(const String& payload, String& error) {
  if (payload.length() > 8192) { error="Backup too large"; return false; }
  DynamicJsonDocument doc(6144);
  if (deserializeJson(doc,payload)) { error="Invalid JSON"; return false; }
  if (doc["schema"] != "cyd-youtube-settings" || doc["version"] != 1) {
    error="Unsupported backup schema"; return false;
  }
  // Reject unexpected root keys to avoid silently accepting secret-bearing files.
  for (JsonPairConst item : doc.as<JsonObjectConst>()) {
    const String key=item.key().c_str();
    if (key!="schema" && key!="version" && key!="refreshMinutes" &&
        key!="quotaLimit" && key!="quotaReserve" && key!="brightness" &&
        key!="rotation" && key!="inverted" && key!="ledEnabled" &&
        key!="ledBrightness" && key!="ledDurationSeconds" &&
        key!="webTheme" && key!="displayTheme" && key!="alerts") {
      error="Unexpected field: "+key; return false;
    }
  }
  if (!doc["refreshMinutes"].is<unsigned int>() ||
      !doc["quotaLimit"].is<unsigned int>() ||
      !doc["quotaReserve"].is<unsigned int>() ||
      !doc["brightness"].is<unsigned int>() ||
      !doc["rotation"].is<unsigned int>() ||
      !doc["ledBrightness"].is<unsigned int>() ||
      !doc["ledDurationSeconds"].is<unsigned int>() ||
      !doc["inverted"].is<bool>() || !doc["ledEnabled"].is<bool>()) {
    error="Invalid field types"; return false;
  }
  const uint32_t minutes=doc["refreshMinutes"], limit=doc["quotaLimit"],
    reserve=doc["quotaReserve"], bright=doc["brightness"],
    rot=doc["rotation"], ledBright=doc["ledBrightness"], duration=doc["ledDurationSeconds"];
  if(minutes<1 || minutes>1440 || limit<1 || limit>1000000 || reserve>60000 ||
     bright>255 || rot>3 || ledBright>255 || duration<1 || duration>120) {
    error="Value outside allowed range"; return false;
  }
  uint32_t newWeb[5], newDisplay[8], newEventColors[EVENT_COUNT];
  bool newEventEnabled[EVENT_COUNT];
  JsonObjectConst w=doc["webTheme"].as<JsonObjectConst>();
  JsonObjectConst d=doc["displayTheme"].as<JsonObjectConst>();
  JsonObjectConst alerts=doc["alerts"].as<JsonObjectConst>();
  if(w.isNull() || d.isNull() || alerts.isNull()) {error="Missing theme/alerts";return false;}
  for(int i=0;i<5;++i) {
    if(!w[kWebKeys[i]].is<const char*>()) {error="Invalid web theme";return false;}
    String c=w[kWebKeys[i]].as<String>();
    if(c.length()!=7 || c[0]!='#') {error="Invalid web color";return false;}
    newWeb[i]=parseHex(c,webColors[i]);
  }
  for(int i=0;i<8;++i) {
    if(!d[kThemeKeys[i]].is<const char*>()) {error="Invalid display theme";return false;}
    String c=d[kThemeKeys[i]].as<String>();
    if(c.length()!=7 || c[0]!='#') {error="Invalid display color";return false;}
    newDisplay[i]=parseHex(c,from565(*kThemeValues[i]));
  }
  for(int i=0;i<EVENT_COUNT;++i) {
    JsonObjectConst e=alerts[kEventKeys[i]].as<JsonObjectConst>();
    if(e.isNull() || !e["enabled"].is<bool>() || !e["color"].is<const char*>()) {
      error="Invalid alert settings";return false;
    }
    String c=e["color"].as<String>();
    if(c.length()!=7 || c[0]!='#') {error="Invalid alert color";return false;}
    newEventEnabled[i]=e["enabled"].as<bool>();
    newEventColors[i]=parseHex(c,eventColors[i]);
  }
  refreshMinutes=minutes; dailyQuotaLimit=limit; quotaReserve=reserve;
  brightness=bright; rotation=rot; panelInverted=doc["inverted"].as<bool>();
  ledEnabled=doc["ledEnabled"].as<bool>(); ledBrightness=ledBright;
  ledDurationSeconds=duration;
  for(int i=0;i<5;++i)webColors[i]=newWeb[i];
  for(int i=0;i<8;++i)*kThemeValues[i]=to565(newDisplay[i]);
  for(int i=0;i<EVENT_COUNT;++i){eventEnabled[i]=newEventEnabled[i];eventColors[i]=newEventColors[i];}
  saveSettings();
  return true;
}

void startConfigServer() {
  WiFi.setHostname(deviceHostname.c_str());
  applyApMode();
  const char* authHeaders[] = {"Authorization", "Origin"};
  web.collectHeaders(authHeaders, 2);
  web.onNotFound([](){ web.sendHeader("Cache-Control","no-store"); web.send(404,"text/plain","Not found"); });
  web.on("/account",HTTP_GET,[](){
    if(!requireAdmin(true))return;
    String h="<!doctype html><html><head><meta name='viewport' content='width=device-width,initial-scale=1'>"
      "<title>Device account</title><style>body{background:#0b1110;color:white;font:16px system-ui;max-width:430px;margin:35px auto;padding:16px}"
      "input,button{display:block;box-sizing:border-box;width:100%;padding:12px;margin:12px 0;border-radius:8px}button{background:#009245;color:white;border:0}</style></head><body>"
      "<h2>Device administrator</h2>";
    if(forcePasswordChange)h+="<p>First sign-in: you must replace the factory admin/admin password before accessing settings.</p>";
    h+="<form method='post' action='/account'>"
      "<label>Username (can be changed later)</label><input name='user' minlength='3' maxlength='32' value='"+escapeHtml(adminUser)+"' required>"
      "<label>New password (12+ characters)</label><input type='password' name='password' minlength='12' maxlength='128' required>"
      "<label>Repeat password</label><input type='password' name='confirm' minlength='12' maxlength='128' required>"
      "<button>Save credentials</button></form></body></html>";
    web.send(200,"text/html; charset=utf-8",h);
  });
  web.on("/account",HTTP_POST,[](){
    if(!requireAdmin(true))return;
    String user=web.arg("user"),pw=web.arg("password");
    if(user.length()<3||user.length()>32||pw.length()<12||pw.length()>128||pw!=web.arg("confirm")) {
      web.send(400,"text/plain","Invalid username, password, or confirmation");return;
    }
    for(size_t i=0;i<user.length();++i)if(!isalnum((unsigned char)user[i])&&user[i]!='_'&&user[i]!='-') {
      web.send(400,"text/plain","Username may contain only letters, numbers, - and _");return;
    }
    adminUser=user;setAdminPassword(pw,false);
    web.send(200,"text/html","<h2>Credentials updated.</h2><p>Open <a href='/'>dashboard</a> and sign in with the new credentials.</p>");
  });
  web.on("/network/config",HTTP_POST,[](){
    if(!requireAdmin())return;
    String host=web.arg("hostname"),name=web.arg("apname"),pass=web.arg("appass");
    host.trim();name.trim();
    if(host.length()<1||host.length()>32||host[0]=='-'||host[host.length()-1]=='-'){
      web.send(400,"text/plain","Hostname must be 1-32 characters and not start/end with a hyphen");return;
    }
    for(size_t i=0;i<host.length();++i){char c=host[i];if(!isalnum((unsigned char)c)&&c!='-'){
      web.send(400,"text/plain","Hostname accepts only ASCII letters, digits and hyphens");return;
    }}
    if(name.length()<1||name.length()>32){web.send(400,"text/plain","AP SSID must be 1-32 characters");return;}
    bool open=web.arg("security")=="open";
    if(!open && pass.length()>0 && (pass.length()<8||pass.length()>63)){
      web.send(400,"text/plain","WPA2 AP password must contain 8-63 characters");return;
    }
    if(!open && pass.isEmpty() && (apOpen || apPassword.length()<8)){
      web.send(400,"text/plain","Set an AP password before enabling WPA2");return;
    }
    int channel=web.arg("channel").toInt(),maxClients=web.arg("clients").toInt();
    if(channel<1||channel>11||maxClients<1||maxClients>8){
      web.send(400,"text/plain","Invalid AP channel or maximum clients");return;
    }
    bool enabled=web.hasArg("enabled");
    if(!enabled && WiFi.status()!=WL_CONNECTED){
      web.send(409,"text/plain","Connect to a local Wi-Fi network before disabling the AP");return;
    }
    deviceHostname=host;apName=name;
    if(pass.length())apPassword=pass;
    apOpen=open;apChannel=(uint8_t)channel;apMaxClients=(uint8_t)maxClients;
    apHidden=web.hasArg("hidden");apEnabled=enabled;
    saveSettings();
    prefs.begin("creator",false);prefs.putBool("apenable",apEnabled);prefs.end();
    web.send(200,"text/html; charset=utf-8","<h2>Network configuration saved</h2><p>Device restarting. Reconnect using the new AP name or LAN IP.</p>");
    delay(500);ESP.restart();
  });
  web.on("/", HTTP_GET, []() {
    if (!requireAdmin()) return;
    String html = F("<!doctype html><html lang='en'><head><meta charset='utf-8'>"
      "<meta name='viewport' content='width=device-width,initial-scale=1'>"
      "<title>CYD YouTube Dashboard</title><style>"
      "body{background:#0b1110;color:#fff;font:15px system-ui,Arial;max-width:480px;margin:25px auto;padding:18px}"
      "section{background:#16241e;border:1px solid #294637;border-radius:14px;padding:18px;margin:16px 0}"
      "label{display:block;margin-top:13px;color:#c2d2c9}input,select{box-sizing:border-box;width:100%;padding:12px;margin:6px 0;background:#0b1110;color:white;border:1px solid #009245;border-radius:8px}"
      "button{background:#009245;color:white;border:0;border-radius:8px;padding:12px;width:100%;margin-top:10px;font-weight:600}"
      "small{color:#a6b9af}h1{font-size:23px}h2{font-size:18px}.bar{height:17px;background:#34463e;border-radius:20px;overflow:hidden}.fill{height:100%;background:#009245}.forecast{background:#6B6BE8}</style></head><body>"
      "<div class='app-shell'><aside class='sidebar'><div class='brand'><span class='brand-mark'>◉</span><span>CYD <small>YouTube Dashboard</small></span></div>"
      "<nav id='main-nav' aria-label='Settings navigation'></nav><div class='sidebar-foot'>CYD • ST7789<br>Standalone dashboard<div class='sidebar-uptime'>Uptime <span id='device-uptime' aria-live='off'>Loading…</span></div></div></aside>"
      "<main class='content'><header class='topbar'><div><span class='eyebrow'>DEVICE CONTROL CENTER</span><h1>Settings &amp; monitoring</h1></div>"
      "<span class='status-dot'>● Local device</span></header><section><h2>Network status</h2><p>");
    // Theme overrides are scoped to CSS variables; escape-free numeric color values only.
    String css = "<style>:root{--web-bg:"+hexColor(webColors[0])+";--web-card:"+hexColor(webColors[1])+";--web-accent:"+hexColor(webColors[2])+";--web-text:"+hexColor(webColors[3])+";--web-secondary:"+hexColor(webColors[4])+"}"
      "body{background:var(--web-bg);color:var(--web-text)}section{background:var(--web-card)}"
      "button,.fill{background:var(--web-accent)}input,select{border-color:var(--web-accent)}"
      "small{color:var(--web-secondary)}</style>";
    html.replace("</head>",css+"</head>");
    String modernCss = R"CSS(<style>
*{box-sizing:border-box}html{scroll-behavior:smooth}body{margin:0!important;padding:0!important;max-width:none!important;font:14px/1.5 system-ui,-apple-system,Segoe UI,sans-serif!important;background:#0b1110;color:#f1f5f2}
.app-shell{display:flex;min-height:100vh}.sidebar{width:248px;position:sticky;top:0;height:100vh;background:#101a17;border-right:1px solid #26362f;padding:25px 15px;display:flex;flex-direction:column;flex-shrink:0}
.brand{display:flex;align-items:center;gap:11px;font-size:14px;font-weight:800;letter-spacing:.08em;padding:6px 12px 28px}.brand small{display:block;font-size:10px;font-weight:500;letter-spacing:.14em;color:#9cb2a7}.brand-mark{font-size:29px;color:var(--web-accent,#009245)}
.sidebar nav{display:grid;gap:5px}.nav-link{display:flex;align-items:center;gap:10px;border-radius:9px;color:#b4c7bd;text-decoration:none;padding:11px 13px;font-size:13px;font-weight:600;transition:background .15s}.nav-link:hover,.nav-link.active{background:#21382e;color:#fff}.nav-link.active{box-shadow:inset 3px 0 var(--web-accent,#009245)}
.sidebar-foot{margin-top:auto;padding:14px 12px;color:#849e90;font-size:11px}.sidebar-uptime{margin-top:12px;padding-top:10px;border-top:1px solid #26362f;display:flex;flex-direction:column;gap:3px}.sidebar-uptime span{color:#c7dfd2;font-variant-numeric:tabular-nums;font-size:12px}.content{max-width:1100px;width:100%;padding:34px 42px 60px;margin:0 auto}.topbar{display:flex;align-items:center;justify-content:space-between;gap:20px;margin-bottom:25px;border-bottom:1px solid #26362f;padding-bottom:23px}.topbar h1{font-size:26px;margin:4px 0 0}.eyebrow{font-size:10px;letter-spacing:.16em;color:var(--web-accent,#009245);font-weight:800}.status-dot{font-size:12px;color:#8fd3ad;white-space:nowrap}
section{margin:0 0 18px!important;padding:25px!important;border-radius:15px!important;box-shadow:0 8px 25px #00000018;scroll-margin-top:18px}section h2{font-size:17px!important;margin:0 0 14px}section h3{font-size:14px;margin:20px 0 7px}label{font-size:12px;font-weight:600}input,select,textarea{border:1px solid #365246!important;border-radius:9px!important;background:#0c1713!important;color:#f4f8f5!important;max-width:100%}input[type=color]{height:46px;padding:5px}button{border-radius:9px!important;cursor:pointer;max-width:100%;transition:filter .15s}button:hover{filter:brightness(1.15)}a{color:#80d4a6}.check{display:flex;align-items:center;gap:10px;margin:12px 0}.check input{width:auto!important;margin:0!important}small{line-height:1.5}section[hidden]{display:none!important}
@media(max-width:760px){.app-shell{display:block}.sidebar{width:100%;height:auto;position:relative;padding:13px;border-right:0;border-bottom:1px solid #26362f}.brand{padding:3px 6px 12px}.sidebar nav{display:flex;overflow:auto;gap:5px}.nav-link{white-space:nowrap;padding:8px 12px}.sidebar-foot{display:block;padding:10px 6px}.sidebar-uptime{margin-top:7px}.content{padding:20px 14px 40px}.topbar h1{font-size:21px}.status-dot{font-size:10px}section{padding:17px!important}}
</style>)CSS";
    html.replace("</head>",modernCss+"<style>"+String(R"CSS(
/* Dense desktop appearance editor; mobile remains scrollable and accessible. */
section:has(.appearance-layout){padding:18px!important}
.appearance-layout{display:grid;grid-template-columns:1fr 1.25fr 1.25fr;gap:14px;align-items:start}
.appearance-group{min-width:0;background:#0d1b16;border:1px solid #294538;border-radius:11px;padding:12px}
.appearance-group h3{margin:0 0 9px!important}
.color-grid{display:grid;grid-template-columns:repeat(2,minmax(0,1fr));gap:7px}
.color-field{display:flex;align-items:center;justify-content:space-between;gap:7px;margin:0;min-width:0;font-size:11px}
.color-field span{overflow-wrap:anywhere}
.appearance-group input[type=color]{width:48px!important;min-width:48px!important;height:29px!important;padding:2px!important;margin:0!important;cursor:pointer}
.appearance-group input[type=number]{height:31px!important;padding:5px!important;margin:2px 0 7px!important}
.event-field{display:flex;align-items:center;gap:7px;margin:7px 0!important;font-size:11px}
.event-field span{flex:1;min-width:0}
.event-field input[type=checkbox]{width:15px!important;flex:none;margin:0!important}
@media(max-width:1150px){.appearance-layout{grid-template-columns:1fr 1fr}.appearance-group:last-child{grid-column:1/-1}}
@media(max-width:760px){.appearance-layout{grid-template-columns:1fr}.appearance-group:last-child{grid-column:auto}}
)CSS")+"</style></head>");

    html += wifiStatusText();
    html += F("<form method='post' action='/network/config'><label>Device hostname</label><input name='hostname' maxlength='32' required value='");
    html += escapeHtml(deviceHostname);
    html += F("'><label>Access point SSID</label><input name='apname' maxlength='32' required value='");
    html += escapeHtml(apName);
    html += F("'><label>AP password (leave blank to retain current)</label><input type='password' name='appass' minlength='8' maxlength='63' autocomplete='new-password' placeholder='Unchanged'>"
      "<label>AP security</label><select name='security'><option value='wpa2'");
    if(!apOpen)html += " selected";
    html += F(">WPA2-Personal (recommended)</option><option value='open'");
    if(apOpen)html += " selected";
    html += F(">Open (no password; insecure)</option></select>"
      "<label>2.4 GHz Wi-Fi channel (1-11)</label><input type='number' name='channel' min='1' max='11' value='");
    html += String(apChannel);
    html += F("'><label>Maximum AP clients (1-8)</label><input type='number' name='clients' min='1' max='8' value='");
    html += String(apMaxClients);
    html += F("'><label class='check'><input type='checkbox' name='hidden' value='1'");
    if(apHidden)html += " checked";
    html += F("> Hide SSID broadcast</label><label class='check'><input type='checkbox' name='enabled' value='1'");
    if(apEnabled)html += " checked";
    html += F("> Enable setup access point</label><button>Save network settings &amp; restart</button></form>"
      "<small>2.4 GHz only. 20/40 MHz channel width is driver-controlled and not exposed. In AP+STA mode the station connection can determine the effective AP channel. WPA3 is not supported by this build.</small>");
    html += F("<p><a href='/account'>Change administrator username/password</a></p>");
    html += F("</p><small>Setup access point address: http://192.168.4.1 (when enabled)</small></section>"
      "<section><h2>Connect to Wi-Fi</h2><form action='/save' method='post'>"
      "<label>Available networks</label><select id='networks' onchange='if(this.value)document.getElementById(\"ssid\").value=this.value'>"
      "<option value=''>Scan for networks...</option></select>"
      "<button type='button' onclick='scan()'>Scan Wi-Fi networks</button>"
      "<label>Network name (SSID)</label><input id='ssid' name='ssid' maxlength='32' required value='");
    html += escapeHtml(wifiName);
    html += F("'><label>Wi-Fi password</label><input name='password' type='password' maxlength='63' placeholder='Leave empty to keep saved password'>"
      "<label>Brightness (0-255)</label><input name='bright' type='number' min='0' max='255' value='");
    html += String(brightness);
    html += F("'><button type='submit'>Save and reconnect</button></form>"
      "<small>Changing networks restarts the device. Its new LAN IP will appear on the screen.</small></section>"
      "<section><h2>Display diagnostics</h2><p>Inversion: ");
    html += panelInverted ? "ON" : "OFF";
    html += F("</p><form action='/display-mode' method='post'><button>Toggle inversion + RGB bars</button></form>"
      "<form action='/display-test' method='post'><button>Show RGB bars</button></form></section>");
    html += F("<section><h2>Active channel branding</h2><p>");
    if (channelAvatarUrl.startsWith("https://")) {
      html += "<img alt='Channel avatar' width='72' height='72' style='border-radius:50%;vertical-align:middle;margin-right:12px' src='" + escapeHtml(channelAvatarUrl) + "'>";
    }
    html += "<b>" + escapeHtml(data.channel) + "</b></p><small>Resolved channel ID: " + escapeHtml(resolvedChannelId) + "</small>";
    html += F("<p>Channel title and avatar are loaded from YouTube. The CYD display shows the live title; its eye icon is shown only when JPEG download or decoding fails.</p></section>");
    html += F("<section><h2>Appearance & RGB alerts</h2><p>Customize both the web dashboard and TFT palette. Changes are saved on-device.</p>"
      "<form method='post' action='/appearance/save'>");
    html += F("<div class='appearance-layout'><div class='appearance-group'><h3>Web colors</h3><div class='color-grid'>");
    const char* webLabels[5] = {"Background","Cards","Accent / buttons","Text","Secondary text"};
    for(int i=0;i<5;++i) {
      html += "<label class='color-field'><span>"+String(webLabels[i])+"</span><input type='color' name='"+String(kWebKeys[i])+"' value='"+hexColor(webColors[i])+"'></label>";
    }
    html += F("</div></div><div class='appearance-group'><h3>CYD screen colors</h3><div class='color-grid'>");
    const char* screenLabels[8] = {"Background","Cards","Green accent","Logo background","Purple accent","Primary text","Muted text","Dividers"};
    for(int i=0;i<8;++i) {
      html += "<label class='color-field'><span>"+String(screenLabels[i])+"</span><input type='color' name='"+String(kThemeKeys[i])+"' value='"+hexColor(from565(*kThemeValues[i]))+"'></label>";
    }
    html += F("</div></div><div class='appearance-group'><h3>RGB LED alerts</h3><label><input type='checkbox' name='ledenable' value='1' style='width:auto'> Enable LED alerts</label>"
      "<label>LED brightness (0-255)</label><input type='number' name='ledbright' min='0' max='255' value='");
    html += String(ledBrightness);
    html += F("'><label>Alert duration (seconds)</label><input type='number' name='leddur' min='1' max='120' value='");
    html += String(ledDurationSeconds);
    html += F("'>");
    const char* eventLabels[EVENT_COUNT] = {"Subscribers increase","Channel views increase","Likes increase (last 10 videos)","Comments increase (last 10 videos)"};
    for(int i=0;i<EVENT_COUNT;++i) {
      html += "<label class='event-field'><input type='checkbox' name='"+String(kEventKeys[i])+"enabled' value='1'"+(eventEnabled[i]?" checked":"")+"><span>"+String(eventLabels[i])+"</span><input type='color' name='"+String(kEventKeys[i])+"color' value='"+hexColor(eventColors[i])+"'></label>";
    }
    // Fix global LED checkbox state in generated form.
    if(ledEnabled) html.replace("name='ledenable' value='1' style='width:auto'>","name='ledenable' value='1' style='width:auto' checked>");
    html += F("</div></div><button type='submit'>Save appearance & LED settings</button></form>"
      "<form method='post' action='/led/test'><button type='submit'>Test RGB LED (5 seconds)</button></form>"
      "<small>LED alerts compare consecutive successful polls, not individual events. No notification is triggered on first sync. Common CYD RGB pins are 4/16/17 (active low).</small></section>");
    html += R"DIAG(<section><h2>Diagnostics</h2><p>Recent device events and optional serial debug output. Only the latest 12 events are retained; the NVS snapshot is written at most once per minute.</p>
<label class='check'><input type='checkbox' id='diag-debug'> Enable serial debug logging (115200 baud)</label>
<div style='display:flex;gap:9px;flex-wrap:wrap'><button type='button' id='diag-save'>Apply logging setting</button><button type='button' id='diag-refresh'>Refresh events</button><button type='button' id='diag-clear'>Clear history</button></div>
<p id='diag-status' role='status'></p><div id='diag-events' style='max-height:310px;overflow:auto;font:12px/1.6 monospace'></div>
<small>Debug output is disabled by default. Event messages exclude credentials, API keys and request URLs. Persistent events are informational; this is not a full crash dump.</small></section>)DIAG";
    html += F("<section><h2>Backup &amp; restore</h2>"
      "<p>Portable settings only. Wi-Fi SSID/password, API key, admin password, history and quota counters are excluded.</p>"
      "<a href='/backup' download='cyd-youtube-settings.json'>Download safe settings backup</a>"
      "<form method='post' action='/restore' enctype='application/x-www-form-urlencoded'>"
      "<label>Paste JSON backup (max 8 KB)</label><textarea name='backup' rows='5' maxlength='8192' "
      "style='width:100%;box-sizing:border-box;background:#0b1110;color:white'></textarea>"
      "<button type='submit'>Restore settings &amp; restart</button></form></section>");
    html += quotaPanel();
    html += R"SNAP(<section id='panel-live'><h2>Display snapshot &amp; remote touch</h2>
<p style='color:#a5b9af'>Actual ST7789 pixel readback, captured on demand. Click the image to send a touch to the CYD, then capture again. No synthetic display rendering or background streaming.</p>
<div style='max-width:700px'><img id='tft-frame' alt='Actual TFT snapshot' draggable='false' style='display:block;width:100%;aspect-ratio:4/3;image-rendering:pixelated;cursor:crosshair;touch-action:manipulation;background:#101010;border:1px solid #3a5b4a;border-radius:8px'></div>
<div style='display:flex;gap:10px;flex-wrap:wrap;align-items:center;margin-top:12px'><label style='display:flex;gap:8px;align-items:center;cursor:pointer'><input type='checkbox' id='tft-invert' style='width:auto;margin:0'> Invert snapshot colors</label><label style='display:flex;gap:6px;align-items:center'>Pixel decoding <select id='tft-color-mode' style='width:auto;min-width:180px'><option value='0'>RGB565 (standard)</option><option value='1'>Swap bytes (reference CYD default)</option><option value='2'>Swap red / blue</option><option value='3'>Swap bytes + red / blue</option></select></label><button type='button' id='tft-capture' style='width:auto'>Capture snapshot</button><button type='button' id='mirror-refresh' style='width:auto'>Refresh statistics</button><span id='tft-capture-status' style='font-size:12px;color:#a5b9af'></span></div>
<small>Snapshot is not a live feed. Click coordinates are mapped to the device's 320 × 240 touch layout. Display changes are visible after the next capture. Only use on a trusted local network.</small></section>)SNAP";
    html += F("<section><h2>YouTube API configuration</h2><form action='/youtube/save' method='post'>"
      "<label>Google API key (blank keeps saved key)</label><input type='password' name='key' autocomplete='new-password'>"
      "<label>Channel handle (@name) or Channel ID (UC...)</label><input name='channel' value='");
    html += escapeHtml(youtubeChannelId);
    html += F("'><label>Refresh interval (minutes)</label><input name='refresh' type='number' min='1' max='1440' value='");
    html += String(refreshMinutes);
    html += F("'><label>Daily quota budget (units)</label><input name='limit' type='number' min='1' max='1000000' value='");
    html += String(dailyQuotaLimit);
    html += F("'><label>Quota reserve (units)</label><input name='reserve' type='number' min='0' max='60000' value='");
    html += String(quotaReserve);
    html += F("'><button type='submit'>Save YouTube settings</button></form>"
      "<form action='/youtube/refresh' method='post'><button>Refresh YouTube now</button></form>"
      "<small>Requests run directly from ESP32. Do not expose this UI to the internet.</small></section>"
      "<script>async function scan(){const list=document.getElementById('networks');"
      "list.replaceChildren(new Option('Scanning...',''));try{const r=await fetch('/api/wifi/scan');"
      "if(!r.ok)throw Error('Scan failed');const networks=await r.json();"
      "list.replaceChildren(new Option('Select a network',''));"
      "for(const n of networks){const label=n.ssid+' ('+n.rssi+' dBm)'+(n.secure?' [locked]':'');"
      "list.add(new Option(label,n.ssid));}if(!networks.length)list.add(new Option('No networks found',''));}"
      "catch(e){list.replaceChildren(new Option('Scan failed; enter SSID manually',''));}}scan();</script></body></html>");
    html.replace("</body></html>",R"NAV(<script>
(function(){const sections=[...document.querySelectorAll('main section')];const nav=document.getElementById('main-nav');const groups=[['Overview','Network status'],['Live display','Live display'],['Wi-Fi','Connect to Wi-Fi'],['Display','Display'],['Channel','Active channel branding'],['Appearance','Appearance & RGB alerts'],['Backup','Backup & restore'],['Quota','Quota'],['YouTube','YouTube API configuration'],['Diagnostics','Diagnostics']];let matched=0;
for(const [label,match] of groups){let s=sections.find(el=>!el.dataset.nav&&el.querySelector('h2')&&el.querySelector('h2').textContent.toLowerCase().includes(match.toLowerCase()));if(!s)continue;s.dataset.nav=label;s.id=s.id||'panel-'+matched++;const a=document.createElement('a');a.href='#'+s.id;a.className='nav-link';a.textContent=label;a.addEventListener('click',()=>{for(const el of sections)el.hidden=el!==s;for(const x of nav.children)x.classList.toggle('active',x===a);});nav.appendChild(a);}
for(const s of sections){if(!s.dataset.nav){s.dataset.nav='More';s.id='panel-'+matched++;const a=document.createElement('a');a.href='#'+s.id;a.className='nav-link';a.textContent=s.querySelector('h2')?.textContent||'More';a.addEventListener('click',()=>{for(const el of sections)el.hidden=el!==s;for(const x of nav.children)x.classList.toggle('active',x===a);});nav.appendChild(a);}}
const hash=location.hash;let first=[...nav.children].find(a=>a.getAttribute('href')===hash)||nav.firstElementChild;if(first)first.click();
})();</script></main></div></body></html>)NAV");
    html.replace("</body></html>",R"SNAPJS(<script>
(()=>{const img=document.getElementById('tft-frame');if(!img)return;
const status=document.getElementById('tft-capture-status');const invert=document.getElementById('tft-invert');const colorMode=document.getElementById('tft-color-mode');let busy=false;
fetch('/api/display/snapshot-settings').then(r=>{if(!r.ok)throw Error();return r.json()}).then(v=>{invert.checked=!!v.inverted;colorMode.value=String(v.mode??0)}).catch(()=>{status.textContent='Could not load snapshot preference';});
async function saveColorSettings(){invert.disabled=true;colorMode.disabled=true;try{const r=await fetch('/api/display/snapshot-settings',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:new URLSearchParams({inverted:invert.checked?'1':'0',mode:colorMode.value})});if(!r.ok)throw Error('HTTP '+r.status);capture();}catch(e){status.textContent='Could not save pixel decoding: '+e.message;}finally{invert.disabled=false;colorMode.disabled=false;}}
invert.addEventListener('change',saveColorSettings);colorMode.addEventListener('change',saveColorSettings);
function capture(){if(busy)return;busy=true;status.textContent='Capturing pixels...';img.onload=()=>{busy=false;status.textContent='Snapshot ready — click image to touch CYD';};img.onerror=()=>{busy=false;status.textContent='Snapshot failed';};img.src='/api/display/frame.bmp?t='+Date.now();}
img.addEventListener('click',async e=>{if(busy||!img.naturalWidth)return;const r=img.getBoundingClientRect();const x=Math.min(319,Math.max(0,Math.floor((e.clientX-r.left)*320/r.width)));const y=Math.min(239,Math.max(0,Math.floor((e.clientY-r.top)*240/r.height)));busy=true;status.textContent='Sending touch '+x+','+y+'...';try{const resp=await fetch('/api/display/touch',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:new URLSearchParams({x:String(x),y:String(y)})});if(!resp.ok)throw Error('HTTP '+resp.status);busy=false;setTimeout(capture,250);}catch(err){busy=false;status.textContent='Touch failed: '+err.message;}});
document.getElementById('tft-capture').onclick=capture;
document.getElementById('mirror-refresh').onclick=async()=>{status.textContent='Statistics refresh requested';try{const r=await fetch('/youtube/stats-refresh',{method:'POST',redirect:'manual'});if(!r.ok&&r.status!==0)throw Error('HTTP '+r.status);status.textContent='Refresh queued; capture again after data updates';}catch(e){status.textContent='Refresh request failed';}};
const link=document.querySelector('a[href="#panel-live"]');if(link)link.addEventListener('click',()=>{if(!img.src)capture();});
if(location.hash==='#panel-live')capture();
})();</script></body></html>)SNAPJS");
    html.replace("</body></html>",R"DIAGJS(<script>
(()=>{const box=document.getElementById('diag-events');if(!box)return;
const enabled=document.getElementById('diag-debug'),status=document.getElementById('diag-status');
async function refresh(){try{const r=await fetch('/api/diagnostics',{cache:'no-store'});if(!r.ok)throw Error('HTTP '+r.status);const d=await r.json();enabled.checked=!!d.debug;box.replaceChildren();
for(const e of d.events){const line=document.createElement('div');line.textContent='['+e.level+'] '+e.uptime_s+'s · '+e.message;box.appendChild(line)}
if(!d.events.length)box.textContent='No events recorded';status.textContent='Showing '+d.events.length+' most recent events';}
catch(e){status.textContent='Could not load diagnostics: '+e.message}}
async function update(action){try{const r=await fetch('/api/diagnostics',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:new URLSearchParams({action,debug:enabled.checked?'1':'0'})});if(!r.ok)throw Error('HTTP '+r.status);await refresh()}catch(e){status.textContent='Diagnostics update failed: '+e.message}}
document.getElementById('diag-save').onclick=()=>update('settings');document.getElementById('diag-clear').onclick=()=>update('clear');document.getElementById('diag-refresh').onclick=refresh;refresh();
})();</script></body></html>)DIAGJS");
    html.replace("</body></html>", R"UPTIME(<script>
(()=>{
const label=document.getElementById('device-uptime');if(!label)return;
let baseSeconds=null, syncTime=0;
function formatUptime(total){const days=Math.floor(total/86400),hours=Math.floor(total%86400/3600),minutes=Math.floor(total%3600/60),seconds=total%60;
const clock=[hours,minutes,seconds].map(n=>String(n).padStart(2,'0')).join(':');return days?days+'d '+clock:clock;}
function render(){if(baseSeconds===null)return;const elapsed=Math.max(0,Math.floor((performance.now()-syncTime)/1000));label.textContent=formatUptime(baseSeconds+elapsed);}
async function sync(){try{const response=await fetch('/api/system/uptime',{cache:'no-store'});if(!response.ok)throw Error('HTTP '+response.status);
const payload=await response.json();if(!Number.isSafeInteger(payload.uptimeSeconds)||payload.uptimeSeconds<0)throw Error('Invalid uptime');
baseSeconds=payload.uptimeSeconds;syncTime=performance.now();render();}catch(e){if(baseSeconds===null)label.textContent='Unavailable';}}
sync();setInterval(render,1000);setInterval(sync,30000);
})();</script></body></html>)UPTIME");
    web.send(200, "text/html; charset=utf-8", html);
  });
  web.on("/api/system/uptime", HTTP_GET, []() {
    if (!requireAdmin()) return;
    // esp_timer_get_time() uses a 64-bit monotonic clock; millis() wraps after ~49 days.
    const uint64_t uptimeSeconds = uint64_t(esp_timer_get_time()) / 1000000ULL;
    web.sendHeader("Cache-Control", "no-store");
    char payload[48];
    snprintf(payload, sizeof(payload), "{\"uptimeSeconds\":%llu}", (unsigned long long)uptimeSeconds);
    web.send(200, "application/json", payload);
  });
  web.on("/api/diagnostics", HTTP_GET, []() {
    if (!requireAdmin()) return;
    DynamicJsonDocument doc(3072);
    doc["debug"] = debugEnabled;
    JsonArray events = doc.createNestedArray("events");
    for (uint8_t i = 0; i < diagnosticCount; ++i) {
      const DiagnosticEntry& entry = diagnosticEntries[(diagnosticHead + kDiagnosticCapacity - diagnosticCount + i) % kDiagnosticCapacity];
      JsonObject item = events.createNestedObject();
      item["uptime_s"] = entry.uptimeSeconds;
      item["level"] = String(entry.level);
      item["message"] = entry.message;
    }
    String output; serializeJson(doc, output);
    web.sendHeader("Cache-Control", "no-store");
    web.send(200, "application/json", output);
  });
  web.on("/api/diagnostics", HTTP_POST, []() {
    if (!requireAdmin()) return;
    if (web.arg("action") == "clear") {
      diagnosticCount = 0; diagnosticHead = 0;
      diagnosticDirty = true;
      persistDiagnostics(true);
    } else if (web.arg("action") == "settings") {
      debugEnabled = web.arg("debug") == "1";
      Preferences diagPrefs;
      if (diagPrefs.begin("cyd-diag", false)) {
        diagPrefs.putBool("debug", debugEnabled);
        diagPrefs.end();
      }
      recordDiagnostic('I', debugEnabled ? "Serial debug enabled" : "Serial debug disabled");
    } else { web.send(400, "text/plain", "Invalid diagnostics action"); return; }
    web.send(200, "application/json", "{\"ok\":true}");
  });
  web.on("/backup", HTTP_GET, []() {
    if (!requireAdmin()) return;
    sendSafeBackup();
  });
  web.on("/restore", HTTP_POST, []() {
    if (!requireAdmin()) return;
    if (!web.hasArg("backup")) {web.send(400,"text/plain","Missing backup JSON");return;}
    String error;
    if(!restoreSafeBackup(web.arg("backup"),error)) {web.send(400,"text/plain",error);return;}
    web.send(200,"text/plain","Settings restored. Rebooting...");
    delay(300); ESP.restart();
  });
  web.on("/appearance/save", HTTP_POST, []() {
    if(!requireAdmin()) return;
    for(int i=0;i<5;++i) if(web.hasArg(kWebKeys[i])) webColors[i]=parseHex(web.arg(kWebKeys[i]),webColors[i]);
    for(int i=0;i<8;++i) if(web.hasArg(kThemeKeys[i])) *kThemeValues[i]=to565(parseHex(web.arg(kThemeKeys[i]),from565(*kThemeValues[i])));
    ledEnabled=web.hasArg("ledenable");
    ledBrightness=uint8_t(constrain(web.arg("ledbright").toInt(),0,255));
    ledDurationSeconds=uint16_t(constrain(web.arg("leddur").toInt(),1,120));
    for(int i=0;i<EVENT_COUNT;++i) {
      eventEnabled[i]=web.hasArg(String(kEventKeys[i])+"enabled");
      if(web.hasArg(String(kEventKeys[i])+"color")) eventColors[i]=parseHex(web.arg(String(kEventKeys[i])+"color"),eventColors[i]);
    }
    if(!ledEnabled) {activeLedEvent=-1;ledOff();}
    saveSettings(); dirty=true;
    web.sendHeader("Location","/",true);web.send(303,"text/plain","");
  });
  web.on("/led/test",HTTP_POST,[](){
    if(!requireAdmin()) return;
    activeLedEvent=SUB_EVENT;ledAlertUntil=millis()+5000UL;
    web.sendHeader("Location","/",true);web.send(303,"text/plain","");
  });
  web.on("/api/wifi/scan", HTTP_GET, []() {
    if (!requireAdmin()) return;
    // The synchronous scan is only initiated by the setup UI, never in the display loop.
    const int count = WiFi.scanNetworks(false, true);
    DynamicJsonDocument json(4096);
    JsonArray networks = json.to<JsonArray>();
    if (count > 0) {
      for (int i = 0; i < count && i < 24; ++i) {
        const String ssid = WiFi.SSID(i);
        if (ssid.isEmpty()) continue;
        JsonObject entry = networks.createNestedObject();
        entry["ssid"] = ssid;
        entry["rssi"] = WiFi.RSSI(i);
        entry["secure"] = WiFi.encryptionType(i) != WIFI_AUTH_OPEN;
      }
    }
    WiFi.scanDelete();
    String response;
    serializeJson(json, response);
    web.send(200, "application/json", response);
  });
  web.on("/api/wifi/status", HTTP_GET, []() {
    if (!requireAdmin()) return;
    DynamicJsonDocument json(384);
    json["connected"] = WiFi.status() == WL_CONNECTED;
    json["ssid"] = WiFi.status() == WL_CONNECTED ? WiFi.SSID() : wifiName;
    json["ip"] = WiFi.status() == WL_CONNECTED ? WiFi.localIP().toString() : "";
    json["setupIp"] = WiFi.softAPIP().toString();
    String response;
    serializeJson(json, response);
    web.send(200, "application/json", response);
  });
  web.on("/save", HTTP_POST, []() {
    if (!requireAdmin()) return;
    const String requestedSsid = web.arg("ssid");
    if (requestedSsid.isEmpty() || requestedSsid.length() > 32) {
      web.send(400, "text/plain", "Invalid SSID");
      return;
    }
    if (requestedSsid != wifiName && !web.hasArg("password")) wifiPassword = "";
    if (requestedSsid != wifiName && web.arg("password").isEmpty()) wifiPassword = "";
    wifiName = requestedSsid;
    if (web.hasArg("password") && web.arg("password").length()) wifiPassword = web.arg("password");
    if (web.hasArg("bright")) brightness = constrain(web.arg("bright").toInt(), 0, 255);
    saveSettings();
    web.send(200, "text/html", "<h2>Saved. Rebooting to connect...</h2><p>Check the CYD screen for its new IP address.</p>");
    delay(500);
    ESP.restart();
  });
  web.on("/display-mode", HTTP_POST, []() {
    if (!requireAdmin()) return;
    panelInverted = !panelInverted;
    display.invertDisplay(panelInverted);
    saveSettings();
    showColorBars();
    colorTestUntil = millis() + 10000;
    web.sendHeader("Location", "/", true);
    web.send(303, "text/plain", "");
  });
  web.on("/display-test", HTTP_POST, []() {
    if (!requireAdmin()) return;
    showColorBars();
    colorTestUntil = millis() + 10000;
    web.sendHeader("Location", "/", true);
    web.send(303, "text/plain", "");
  });
  web.on("/youtube/save", HTTP_POST, []() {
    if (!requireAdmin()) return;
    String key = web.arg("key");
    String chan = web.arg("channel");
    chan.trim();
    uint32_t minutes = web.arg("refresh").toInt();
    uint32_t limit = strtoul(web.arg("limit").c_str(), nullptr, 10);
    uint32_t reserve = strtoul(web.arg("reserve").c_str(), nullptr, 10);
    if (minutes < 1 || minutes > 1440 || limit < 1 || limit > 1000000 || reserve > 60000 ||
        chan.length() > 80 || key.length() > 200) {
      web.send(400, "text/plain", "Invalid YouTube configuration"); return;
    }

    if (key.length()) youtubeKey = key;
    if (youtubeChannelId != chan) {
      avatarRefreshPending = true;
      cachedAvatarUrl = "";
      if (avatarJpeg) { free(avatarJpeg); avatarJpeg = nullptr; avatarJpegLength = 0; }
    }
    youtubeChannelId = chan;
    refreshMinutes = minutes;
    dailyQuotaLimit = limit;
    quotaReserve = reserve;

    saveSettings();
    firstPollPending = true;
    web.sendHeader("Location", "/", true);
    web.send(303, "text/plain", "");
  });
  // Statistics-only refresh. It does not request the channel avatar.
  web.on("/youtube/stats-refresh", HTTP_POST, []() {
    if (!requireAdmin()) return;
    firstPollPending = true;
    web.sendHeader("Location", "/#panel-live", true);
    web.send(303, "text/plain", "");
  });
  // Browser mirror is a semantic representation of the active 320x240 page,
  // not a pixel framebuffer. All values come from the same state as TFT render().
  // EXPERIMENTAL TFT hardware readback. ST7789 readback requires a physically
  // connected TFT SDO -> ESP32 MISO wire. Many CYD variants omit this wire.
  // Streams a 24-bit BMP row by row; no framebuffer allocation is required.
  // Remote touch uses exactly the same hit testing as the physical touchscreen.
  web.on("/api/display/touch", HTTP_POST, []() {
    if (!requireAdmin()) return;
    if (!web.hasArg("x") || !web.hasArg("y")) {web.send(400,"text/plain","Missing coordinates");return;}
    const String sx=web.arg("x"), sy=web.arg("y");
    auto valid=[](const String& v){if(v.isEmpty()||v.length()>3)return false;for(size_t i=0;i<v.length();++i)if(v[i]<'0'||v[i]>'9')return false;return true;};
    if (!valid(sx)||!valid(sy)||sx.toInt()>319||sy.toInt()>239) {web.send(400,"text/plain","Invalid coordinates");return;}
    processScreenTouch(sx.toInt(),sy.toInt());
    if(dirty) render();
    web.sendHeader("Cache-Control","no-store");
    web.send(200,"application/json","{\"ok\":true}");
  });
  web.on("/api/display/snapshot-settings", HTTP_GET, []() {
    if (!requireAdmin()) return;
    web.sendHeader("Cache-Control", "no-store");
    web.send(200, "application/json", String("{\"inverted\":") + (snapshotInverted ? "true" : "false") + ",\"mode\":" + String(snapshotColorMode) + "}");
  });
  web.on("/api/display/snapshot-settings", HTTP_POST, []() {
    if (!requireAdmin()) return;
    if (!web.hasArg("inverted") || (web.arg("inverted") != "0" && web.arg("inverted") != "1")) {
      web.send(400, "text/plain", "Invalid inverted value"); return;
    }
    if (!web.hasArg("mode") || web.arg("mode").length()!=1 || web.arg("mode")[0]<'0' || web.arg("mode")[0]>'3') {web.send(400,"text/plain","Invalid color mode");return;}
    snapshotColorMode = uint8_t(web.arg("mode")[0]-'0');
    snapshotInverted = web.arg("inverted") == "1";
    prefs.begin("creator", false);
    prefs.putBool("snapinvert", snapshotInverted);
    prefs.putUChar("snapmode", snapshotColorMode);
    prefs.end();
    web.sendHeader("Cache-Control", "no-store");
    web.send(200, "application/json", "{\"ok\":true}");
  });
  web.on("/api/display/frame.bmp", HTTP_GET, []() {
    if (!requireAdmin()) return;
    constexpr int W=320, H=240, STRIDE=W*3;
    constexpr uint32_t SIZE=54+STRIDE*H;
    uint8_t header[54]={0};
    header[0]='B'; header[1]='M';
    auto put32=[&](int off,uint32_t v){for(int i=0;i<4;++i)header[off+i]=(v>>(8*i))&255;};
    auto put16=[&](int off,uint16_t v){header[off]=v&255;header[off+1]=v>>8;};
    put32(2,SIZE);put32(10,54);put32(14,40);put32(18,W);put32(22,H);
    put16(26,1);put16(28,24);put32(34,STRIDE*H);
    web.sendHeader("Cache-Control","no-store, max-age=0");
    web.setContentLength(SIZE);
    web.send(200,"image/bmp","");
    WiFiClient client=web.client();
    client.write(header,sizeof(header));
    uint16_t pixels[W];
    uint8_t row[STRIDE];
    for(int y=H-1;y>=0 && client.connected();--y){
      display.readRect(0,y,W,1,pixels);
      for(int x=0;x<W;++x){
        uint16_t v=pixels[x];
        if(snapshotColorMode & 1) v=uint16_t((v<<8)|(v>>8));
        uint8_t r=uint8_t((v>>11)&31), g=uint8_t((v>>5)&63), b=uint8_t(v&31);
        if(snapshotColorMode & 2) {uint8_t tmp=r;r=b;b=tmp;}
        const uint8_t mask=snapshotInverted ? 0xFF : 0;
        row[x*3+0]=uint8_t((b<<3)|(b>>2)) ^ mask;
        row[x*3+1]=uint8_t((g<<2)|(g>>4)) ^ mask;
        row[x*3+2]=uint8_t((r<<3)|(r>>2)) ^ mask;
      }
      client.write(row,sizeof(row));
      if((y&15)==0)delay(0);
    }
  });
  web.on("/api/display/state", HTTP_GET, []() {
    if (!requireAdmin()) return;
    DynamicJsonDocument doc(4096);
    doc["revision"] = screenRevision;
    doc["page"] = page;
    doc["channel"] = data.channel;
    doc["online"] = WiFi.status() == WL_CONNECTED;
    doc["ip"] = doc["online"].as<bool>() ? WiFi.localIP().toString() : WiFi.softAPIP().toString();
    doc["live"] = data.live;
    doc["status"] = youtubeStatus;
    doc["refreshMinutes"] = refreshMinutes;
    doc["lastSyncUptimeSeconds"] = lastSuccessfulStatisticsMs ? (millis()-lastSuccessfulStatisticsMs)/1000 : -1;
    JsonArray cards = doc.createNestedArray("cards");
    const Metric* metrics[] = {&data.subscribers,&data.views,&data.best,&data.latest,&data.likes,&data.comments};
    for (uint8_t i=0;i<6;++i) {
      JsonObject card = cards.createNestedObject();
      card["label"] = metrics[i]->label;
      card["value"] = metrics[i]->value;
      card["change"] = metrics[i]->change;
    }
    const VideoStats& video = page==3 ? topVideo : latestVideo;
    JsonObject v = doc.createNestedObject("video");
    v["valid"] = video.valid;
    v["title"] = video.title;
    v["views"] = compactNumber(video.views);
    v["likes"] = compactNumber(video.likes);
    v["comments"] = compactNumber(video.comments);
    v["published"] = video.published;
    v["duration"] = video.duration;
    doc["periodDays"] = historyPeriods[historyPeriodIndex];
    String out;serializeJson(doc,out);
    web.sendHeader("Cache-Control","no-store");
    web.send(200,"application/json",out);
  });
  web.on("/api/display/navigate", HTTP_POST, []() {
    if (!requireAdmin()) return;
    if (!web.hasArg("page")) { web.send(400,"text/plain","Missing page");return; }
    const String requested=web.arg("page");
    if (requested.length()!=1 || requested[0]<'0' || requested[0]>'6') {web.send(400,"text/plain","Invalid page");return;}
    page=(uint8_t)(requested[0]-'0');
    if(page) metricForPage(page).unread=0;
    dirty=true;
    web.sendHeader("Cache-Control","no-store");
    web.send(200,"application/json","{\"ok\":true}");
  });
  web.on("/youtube/refresh", HTTP_POST, []() {
    if (!requireAdmin()) return;
    avatarRefreshPending = true;
    firstPollPending = true;
    web.sendHeader("Location", "/", true);
    web.send(303, "text/plain", "");
  });
  web.on("/api/youtube/quota", HTTP_GET, []() {
    if (!requireAdmin()) return;
    rolloverQuota();
    DynamicJsonDocument doc(768);
    doc["source"] = "local_accounting_not_google_official";
    doc["usedUnits"] = quotaUsed;
    doc["limitUnits"] = dailyQuotaLimit;
    doc["remainingEstimated"] = dailyQuotaLimit > quotaUsed ? dailyQuotaLimit - quotaUsed : 0;
    doc["projectedFullDayUnits"] = projectedQuota();
    doc["requestsSinceBoot"] = youtubeRequests;
    doc["status"] = youtubeStatus;
    String payload;
    serializeJson(doc, payload);
    web.send(200, "application/json", payload);
  });
  web.begin();
}



void markRead(uint8_t p) {
  Metric& m = metricForPage(p);
  m.unread = 0;

}

void processScreenTouch(int x, int y) {
  x=constrain(x,0,319); y=constrain(y,0,239);
  if (page != 0) {
    if (x < 90 && y < 105) { page = 0; dirty = true; return; }
    if ((page == 1 || page == 2) && y >= 143 && y <= 178) {
      if (x >= 20 && x <= 75) {
        historyPeriodIndex = (historyPeriodIndex + 5) % 6;
        dirty = true;
      } else if (x >= 235 && x <= 300) {
        historyPeriodIndex = (historyPeriodIndex + 1) % 6;
        dirty = true;
      }
    }
    return;
  }
  // Touch the status header on the home page to request statistics only.
  if (y < 55) { if (WiFi.status() == WL_CONNECTED) firstPollPending = true; return; }
  if (y > 215) return;
  if (y < 136) page = (x < 160) ? 1 : 2;
  else page = (x < 160) ? 3 : 4;
  markRead(page);
  dirty = true;
}

void handleTouch() {
  if (!touch.touched() || millis() - lastTouch < kTouchDebounceMs) return;
  TS_Point point = touch.getPoint();
  const int x = constrain(map(point.x, rawMinX, rawMaxX, 0, 320), 0, 319);
  const int y = constrain(map(point.y, rawMinY, rawMaxY, 0, 240), 0, 239);
  lastTouch = millis();
  processScreenTouch(x,y);
}

} // namespace

void setup() {
  Serial.begin(115200);
  loadSettings();
  loadDiagnostics();
  recordDiagnostic('I', "Device booted");
  initPwm();
  ledOff();
  applyBrightness();
  display.init();
  TJpgDec.setCallback(avatarBlock);
  TJpgDec.setSwapBytes(true);
  display.setRotation(rotation);
#ifdef ST7789_DRIVER
  display.invertDisplay(panelInverted);
  Serial.printf("ST7789 inversion: %s\n", panelInverted ? "ON" : "OFF");
#else
  display.invertDisplay(false);
#endif
  showColorBars();
  delay(1800);
  display.fillScreen(kBackground);
  touchBus.begin(kTouchSck, kTouchMiso, kTouchMosi, kTouchCs);
  touch.begin(touchBus);
  touch.setRotation(1);
  // Pacific timezone with DST; quota resets on the API project's Pacific calendar day.
  setenv("TZ", "PST8PDT,M3.2.0,M11.1.0", 1);
  tzset();
  configTime(0, 0, "pool.ntp.org", "time.google.com");
  startConfigServer();
  wifiDisconnectedSince = millis();
  if (wifiName.length()) WiFi.begin(wifiName.c_str(), wifiPassword.c_str());
  render();
  Serial.println("CYD YouTube Dashboard v0.9.6.2 Fast Startup RC ready");
  Serial.println(apEnabled ? "Setup AP enabled" : "Setup AP disabled");
}

void loop() {
  web.handleClient();
  persistDiagnostics();
  updateLed();
  if (colorTestUntil && (int32_t)(millis() - colorTestUntil) < 0) {
    delay(10);
    return;
  }
  if (colorTestUntil) {
    colorTestUntil = 0;
    dirty = true;
  }
  handleTouch();
  const wl_status_t currentWifiStatus = WiFi.status();
  if (currentWifiStatus != previousWifiStatus) {
    previousWifiStatus = currentWifiStatus;
    dirty = true;
    if (currentWifiStatus == WL_CONNECTED) {
      Serial.printf("Connected to %s; IP: %s\n", WiFi.SSID().c_str(), WiFi.localIP().toString().c_str());
      firstPollPending = true;
      recordDiagnostic('I', "Wi-Fi connected");
    } else {
      recordDiagnostic('W', "Wi-Fi disconnected");
    }
  }
  // Recovery AP is independent of the user's saved AP preference.
  // Bring it up after a sustained STA outage, and remove it when STA returns.
  // No credentials or AP preference are modified in persistent storage.
  if (currentWifiStatus == WL_CONNECTED) {
    wifiDisconnectedSince = 0;
    if (recoveryApActive) {
      recoveryApActive = false;
      applyApMode();
      recordDiagnostic('I', "Recovery AP stopped: Wi-Fi restored");
    }
  } else {
    if (wifiDisconnectedSince == 0) wifiDisconnectedSince = millis() ? millis() : 1;
    if (!apEnabled && !recoveryApActive &&
        uint32_t(millis() - wifiDisconnectedSince) >= kRecoveryApDelayMs) {
      recoveryApActive = true;
      applyApMode();
      recordDiagnostic('W', "Recovery AP enabled: Wi-Fi unavailable");
    }
  }
  if (wifiName.length() && WiFi.status() != WL_CONNECTED && millis() - lastWifiAttempt > 15000) {
    lastWifiAttempt = millis();
    WiFi.reconnect();
  }
  // Never consume the boot refresh while STA is disconnected.
  // Retry failed startup polls quickly rather than waiting the full interval.
  if (WiFi.status() == WL_CONNECTED &&
      (firstPollPending || millis() - lastPoll >= uint32_t(refreshMinutes) * 60000UL)) {
    firstPollPending = false;
    lastPoll = millis();
    pollYouTube();
    if (!data.live && !youtubeKey.isEmpty() && !youtubeChannelId.isEmpty()) {
      // A failed poll is retried in 15 seconds, not after refreshMinutes.
      lastPoll = millis() - uint32_t(refreshMinutes) * 60000UL + 15000UL;
    }
  }
  if (dirty) {
    render();
    lastFrame = millis();
  } else if (page == 0 &&
             (data.subscribers.unread || data.views.unread ||
              data.best.unread || data.latest.unread) &&
             millis() - lastFrame >= kGlowFrameMs) {
    lastFrame = millis();
    animateUnread();
  }
  delay(5);
}
