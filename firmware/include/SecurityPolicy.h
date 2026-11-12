#pragma once
#include <Arduino.h>
#include <time.h>
namespace SecurityPolicy {
inline bool clockReady() { return time(nullptr) >= 1700000000; }
inline bool isGoogleImageHost(const String& url) {
  if (!url.startsWith("https://")) return false;
  int start = 8, end = url.indexOf('/', start);
  if (end < 0) end = url.length();
  String host = url.substring(start, end);
  host.toLowerCase();
  if (host.indexOf('@') >= 0 || host.indexOf(':') >= 0 || host.isEmpty()) return false;
  return host == "ggpht.com" || host.endsWith(".ggpht.com") ||
         host == "googleusercontent.com" || host.endsWith(".googleusercontent.com");
}
inline bool sameOrigin(const String& origin, const String& host) {
  if (origin.isEmpty() || host.isEmpty()) return false;
  return origin == "http://" + host;
}
}
