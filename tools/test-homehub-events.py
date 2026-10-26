#!/usr/bin/env python3
"""Compile the actual event store with minimal host-only platform declarations."""
import pathlib
import subprocess
import tempfile

ROOT = pathlib.Path(__file__).resolve().parents[1]
with tempfile.TemporaryDirectory(prefix="homehub-events-") as directory:
    folder = pathlib.Path(directory)
    (folder / "Arduino.h").write_text("""
#pragma once
#include <stdint.h>
#include <stddef.h>
#include <string.h>
class String {};
inline uint32_t millis() { return 1234; }
""")
    (folder / "Preferences.h").write_text("#pragma once\nclass Preferences {};\n")
    (folder / "WiFi.h").write_text("""#pragma once
enum { WL_IDLE_STATUS=0, WL_NO_SSID_AVAIL=1, WL_CONNECTED=3,
       WL_CONNECT_FAILED=4, WL_CONNECTION_LOST=5, WL_DISCONNECTED=6 };
""")
    (folder / "test.cpp").write_text("""
#include "HomeHubStore.h"
#include "HomeHubNetwork.h"
#include <cassert>
int main() {
  SensorRecord added{};
  added.radioAddress = UINT16_MAX;
  addOwnedSensor(added);
  assert(added.flags & kSensorClaimed);
  assert(added.flags & kSensorAwaitingReport);
  markSensorReported(added);
  assert(added.flags & kSensorClaimed);
  assert(!(added.flags & kSensorAwaitingReport));
  // Adding a discovered/ignored sensor preserves its data and restores visibility.
  SensorRecord discovered{};
  discovered.flags = kSensorIgnored | kSensorDescriptorReceived;
  strcpy(discovered.name, "Demo gate");
  strcpy(discovered.primaryValue, "OPEN");
  discovered.lastSeenMs = 123;
  addOwnedSensor(discovered);
  markSensorReported(discovered);
  assert(discovered.flags == (kSensorClaimed | kSensorDescriptorReceived));
  assert(strcmp(discovered.name, "Demo gate") == 0);
  assert(strcmp(discovered.primaryValue, "OPEN") == 0);
  assert(strcmp(homeHubWifiState(WL_CONNECTED, true, 40000), "connected") == 0);
  assert(strcmp(homeHubWifiState(WL_NO_SSID_AVAIL, true, 2000), "not_found") == 0);
  assert(strcmp(homeHubWifiState(WL_CONNECT_FAILED, true, 2000), "failed") == 0);
  assert(strcmp(homeHubWifiState(WL_DISCONNECTED, true, 29999), "connecting") == 0);
  assert(strcmp(homeHubWifiState(WL_DISCONNECTED, true, 30000), "timeout") == 0);
  assert(strcmp(homeHubWifiState(WL_DISCONNECTED, false, 40000), "disconnected") == 0);
  HomeHubStore store;
  assert(store.eventCount() == 0);
  for (int i = 0; i < 100; ++i) store.recordEvent("DEMO-SENSOR", "State: open");
  assert(store.eventCount() == 32);
  for (size_t i = 0; i < 32; ++i) {
    assert(store.eventAt(i).sequence == 69 + i);
    assert(store.eventAt(i).receivedMs == 1234);
    assert(strcmp(store.eventAt(i).sensorId, "DEMO-SENSOR") == 0);
  }
  char longText[200]; memset(longText, 'x', 199); longText[199] = 0;
  store.recordEvent(longText, longText);
  assert(strlen(store.eventAt(31).sensorId) == 24);
  assert(strlen(store.eventAt(31).message) == 80);
  assert(store.eventAt(0).sequence == 70);
}
""")
    executable = folder / "test"
    subprocess.run(["c++", "-std=c++17", "-I", str(folder), "-I",
                    str(ROOT / "firmware/HomeHub/src"), str(folder / "test.cpp"),
                    "-o", str(executable)], check=True)
    subprocess.run([str(executable)], check=True)
print("PASS: sensor ownership survives reports; Wi-Fi outcomes/deadline; bounded event buffer")
