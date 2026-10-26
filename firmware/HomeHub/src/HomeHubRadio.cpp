#include "HomeHubRadio.h"

#include <ctype.h>
#include <stdlib.h>

namespace {
constexpr uint32_t kLoraBaudRate = 115'200;
constexpr uint8_t kLoraTxPin = D6;
constexpr uint8_t kLoraRxPin = D7;
constexpr size_t kMaximumPayload = 220;

bool validId(const char* id) {
  if (id == nullptr || id[0] == '\0' || strnlen(id, 25) >= 25) return false;
  for (const unsigned char* cursor = reinterpret_cast<const unsigned char*>(id);
       *cursor != 0; ++cursor) {
    if (!isalnum(*cursor) && *cursor != '-' && *cursor != '_') return false;
  }
  return true;
}

bool safeText(const char* value, size_t limit) {
  const size_t length = strnlen(value, limit + 1);
  if (length == 0 || length > limit) return false;
  for (size_t index = 0; index < length; ++index) {
    const unsigned char character = value[index];
    if (character < 0x20 || character > 0x7e || character == '|') return false;
  }
  return true;
}

size_t splitFields(char* payload, char** fields, size_t capacity) {
  size_t count = 0;
  char* cursor = payload;
  while (cursor != nullptr && count < capacity) {
    fields[count++] = cursor;
    char* delimiter = strchr(cursor, '|');
    if (delimiter == nullptr) break;
    *delimiter = '\0';
    cursor = delimiter + 1;
  }
  return count;
}

SensorRecord emptySensor(const char* id) {
  SensorRecord sensor{};
  strlcpy(sensor.id, id, sizeof(sensor.id));
  strlcpy(sensor.name, id, sizeof(sensor.name));
  sensor.radioAddress = UINT16_MAX;
  sensor.flags = 0;
  return sensor;
}
}  // namespace

void HomeHubRadio::begin() {
  Serial1.begin(kLoraBaudRate, SERIAL_8N1, kLoraRxPin, kLoraTxPin);
}

void HomeHubRadio::poll() {
  while (Serial1.available() > 0) {
    const char character = static_cast<char>(Serial1.read());
    if (character == '\r') continue;
    if (character == '\n') {
      if (!lineOverflow_ && lineLength_ > 0) {
        line_[lineLength_] = '\0';
        processLine(line_);
      }
      lineLength_ = 0;
      lineOverflow_ = false;
      continue;
    }
    if (lineOverflow_) continue;
    if (lineLength_ + 1 >= sizeof(line_)) {
      lineLength_ = 0;
      lineOverflow_ = true;
      continue;
    }
    line_[lineLength_++] = character;
  }
}

void HomeHubRadio::processLine(char* line) {
  if (strncmp(line, "+RCV=", 5) != 0) return;
  char* sourceEnd = strchr(line + 5, ',');
  if (sourceEnd == nullptr) return;
  char* lengthEnd = strchr(sourceEnd + 1, ',');
  if (lengthEnd == nullptr) return;
  *sourceEnd = '\0';
  *lengthEnd = '\0';
  char* parseEnd = nullptr;
  const long source = strtol(line + 5, &parseEnd, 10);
  if (*parseEnd != '\0' || source < 0 || source > 65535) return;
  const long length = strtol(sourceEnd + 1, &parseEnd, 10);
  if (*parseEnd != '\0' || length <= 0 || length > kMaximumPayload) return;
  char* payload = lengthEnd + 1;
  if (strlen(payload) < static_cast<size_t>(length)) return;
  payload[length] = '\0';
  processPayload(payload, static_cast<uint16_t>(source));
}

void HomeHubRadio::processPayload(char* payload, uint16_t radioAddress) {
  char* fields[10]{};
  const size_t count = splitFields(payload, fields, 10);
  if (count < 2 || strcmp(fields[0], "FS2") != 0) return;

  if (strcmp(fields[1], "SETUP") == 0 && count == 9 && strcmp(fields[3], "1") == 0 &&
      validId(fields[2]) && safeText(fields[4], 16) && safeText(fields[5], 32) &&
      safeText(fields[6], 48) && safeText(fields[7], 16) && safeText(fields[8], 32)) {
    SensorRecord* existing = store_.findSensor(fields[2]);
    SensorRecord sensor = existing == nullptr ? emptySensor(fields[2]) : *existing;
    const bool isNew = existing == nullptr;
    if (isNew || strcmp(sensor.name, sensor.id) == 0 || sensor.name[0] == '\0') {
      strlcpy(sensor.name, fields[5], sizeof(sensor.name));
    }
    const bool descriptorChanged = isNew || strcmp(sensor.type, fields[4]) != 0 ||
        strcmp(sensor.description, fields[6]) != 0 || strcmp(sensor.primaryLabel, fields[7]) != 0;
    strlcpy(sensor.type, fields[4], sizeof(sensor.type));
    strlcpy(sensor.description, fields[6], sizeof(sensor.description));
    strlcpy(sensor.primaryLabel, fields[7], sizeof(sensor.primaryLabel));
    sensor.radioAddress = radioAddress;
    sensor.flags |= kSensorDescriptorReceived;
    sensor.flags &= static_cast<uint8_t>(~kSensorAwaitingReport);
    if (!store_.upsertSensor(sensor, isNew || descriptorChanged)) return;
    store_.recordEvent(sensor.id, "Setup information received");
    sendSetupStatus(sensor);
    return;
  }

  if (strcmp(fields[1], "GATE") == 0 && count == 8 && validId(fields[2]) &&
      (strcmp(fields[5], "0") == 0 || strcmp(fields[5], "1") == 0)) {
    SensorRecord* sensor = store_.findSensor(fields[2]);
    if (sensor == nullptr) {
      SensorRecord discovered = emptySensor(fields[2]);
      strlcpy(discovered.type, "UNKNOWN", sizeof(discovered.type));
      strlcpy(discovered.primaryLabel, "State", sizeof(discovered.primaryLabel));
      if (!store_.upsertSensor(discovered, true)) return;
      sensor = store_.findSensor(fields[2]);
    }
    const char* label = strcmp(sensor->type, "GATE") == 0 ? "Gate state" : "State";
    const char* value = fields[6][0] != '\0' ? fields[6] : fields[5];
    if (!safeText(value, 32)) return;
    if (store_.updateLiveValue(fields[2], label, value, millis(), radioAddress)) {
      char message[81];
      snprintf(message, sizeof(message), "%s: %s", label, value);
      store_.recordEvent(fields[2], message);
    }
    if (strcmp(fields[4], "EVENT") == 0) {
      sendPayload(radioAddress, String("FS2|RCPT|") + fields[2] + "|" + fields[3] + "|OK");
    }
    if ((sensor->flags & kSensorSetupComplete) == 0) sendSetupStatus(*sensor);
  }

  if (strcmp(fields[1], "SETUP_ACK") == 0 && count == 5 && validId(fields[2]) &&
      strcmp(fields[3], "1") == 0 &&
      (strcmp(fields[4], "PENDING") == 0 || strcmp(fields[4], "COMPLETE") == 0)) {
    const SensorRecord* sensor = store_.findSensor(fields[2]);
    if (sensor == nullptr) return;
    const bool complete = strcmp(fields[4], "COMPLETE") == 0 &&
        (sensor->flags & kSensorClaimed) && (sensor->flags & kSensorDescriptorReceived);
    store_.setSensorFlag(fields[2], kSensorSetupComplete, complete);
    if (store_.findSensor(fields[2]) != nullptr)
      store_.recordEvent(fields[2], complete ?
          "Sensor confirmed setup complete" : "Sensor setup pending");
  }
}

bool HomeHubRadio::sendPayload(uint16_t radioAddress, const String& payload) {
  if (radioAddress == UINT16_MAX || payload.length() == 0 ||
      payload.length() > kMaximumPayload) {
    return false;
  }
  Serial1.print("AT+SEND=");
  Serial1.print(radioAddress);
  Serial1.print(',');
  Serial1.print(payload.length());
  Serial1.print(',');
  Serial1.print(payload);
  Serial1.print("\r\n");
  return true;
}

void HomeHubRadio::sendSetupStatus(const SensorRecord& sensor) {
  const char* status = (sensor.flags & kSensorClaimed) &&
      (sensor.flags & kSensorDescriptorReceived) ? "COMPLETE" : "PENDING";
  sendPayload(sensor.radioAddress, String("FS2|SETUP_STATUS|") + sensor.id + "|1|" + status);
}
