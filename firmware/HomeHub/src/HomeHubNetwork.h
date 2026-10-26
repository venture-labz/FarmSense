#pragma once
#include <stdint.h>
#include <WiFi.h>

// Pure status classification: association is not an internet/cloud test.
inline const char* homeHubWifiState(int status, bool attempted, uint32_t elapsed) {
  if (status == WL_CONNECTED) return "connected";
  if (status == WL_NO_SSID_AVAIL) return "not_found";
  if (status == WL_CONNECT_FAILED) return "failed";
  if (attempted) return elapsed < 30000 ? "connecting" : "timeout";
  return "disconnected";
}
