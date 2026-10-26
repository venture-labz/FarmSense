#pragma once

#include <Arduino.h>
#include <Preferences.h>

constexpr size_t kHomeHubMaximumSensors = 32;
constexpr uint8_t kSensorClaimed = 0x01;
constexpr uint8_t kSensorIgnored = 0x02;
constexpr uint8_t kSensorSetupComplete = 0x04;
constexpr uint8_t kSensorAwaitingReport = 0x08;
constexpr uint8_t kSensorDescriptorReceived = 0x10;
constexpr uint8_t kSensorAlarmEnabled = 0x20;
constexpr uint8_t kSensorHasLocation = 0x40;

struct SensorRecord {
  char id[25];
  char type[17];
  char name[33];
  char description[49];
  char grouping[25];
  char location[49];
  char primaryLabel[17];
  char primaryValue[33];
  int32_t latitudeE6;
  int32_t longitudeE6;
  uint32_t lastSeenMs;
  uint16_t radioAddress;
  uint8_t flags;
};

// An explicit Add Sensor is an ownership choice, not just a transient wait.
inline void addOwnedSensor(SensorRecord& sensor) {
  sensor.flags |= kSensorClaimed;
  sensor.flags &= static_cast<uint8_t>(~(kSensorIgnored | kSensorSetupComplete));
  if (sensor.lastSeenMs == 0 && !(sensor.flags & kSensorDescriptorReceived))
    sensor.flags |= kSensorAwaitingReport;
}

inline void markSensorReported(SensorRecord& sensor) {
  sensor.flags &= static_cast<uint8_t>(~kSensorAwaitingReport);
}

struct HubEvent {
  uint32_t sequence;
  uint32_t receivedMs;
  char sensorId[25];
  char message[81];
};
constexpr size_t kHubEventCapacity = 32;
static_assert(sizeof(HubEvent) * kHubEventCapacity <= 4096, "Bound event memory");

class HomeHubStore {
 public:
  void begin();
  bool isReady() const { return ready_; }
  bool isRegistryReady() const { return registryReady_; }
  bool hasAccount() const { return accountReady_; }
  bool createAccount(const String& username, const String& password);
  bool verifyAccount(const String& username, const String& password);
  String startSession();
  bool isSessionValid(const String& token) const;
  void endSession();
  void expireSession();

  size_t sensorCount() const { return sensorCount_; }
  const SensorRecord* sensorAt(size_t index) const;
  SensorRecord* findSensor(const char* id);
  bool setSensorFlag(const char* id, uint8_t flag, bool enabled);
  bool updateLiveValue(const char* id, const char* label, const char* value,
                       uint32_t receivedMs, uint16_t radioAddress);
  bool upsertSensor(const SensorRecord& sensor, bool persist);
  bool saveRegistry();
  bool saveRouterCredentials(const String& ssid, const String& password);
  bool loadRouterCredentials(String& ssid, String& password);
  void recordEvent(const char* id, const char* message) {
    HubEvent& event = events_[eventSequence_ % kHubEventCapacity];
    event.sequence = ++eventSequence_;
    event.receivedMs = millis();
    strlcpy(event.sensorId, id, sizeof(event.sensorId));
    strlcpy(event.message, message, sizeof(event.message));
    if (eventCount_ < kHubEventCapacity) ++eventCount_;
  }
  size_t eventCount() const { return eventCount_; }
  const HubEvent& eventAt(size_t index) const {
    return events_[(eventSequence_ - eventCount_ + index) % kHubEventCapacity];
  }

 private:
  bool derivePassword(const String& password, const uint8_t* salt,
                      uint8_t* output) const;
  bool loadRegistry();

  Preferences preferences_;
  HubEvent events_[kHubEventCapacity]{};
  uint32_t eventSequence_ = 0;
  size_t eventCount_ = 0;
  SensorRecord sensors_[kHomeHubMaximumSensors]{};
  size_t sensorCount_ = 0;
  bool ready_ = false;
  bool registryReady_ = true;
  bool accountReady_ = false;
  uint8_t salt_[16]{};
  uint8_t passwordHash_[32]{};
  char username_[25]{};
  char session_[33]{};
  uint32_t sessionStartedMs_ = 0;
};
