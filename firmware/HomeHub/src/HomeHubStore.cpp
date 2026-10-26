#include "HomeHubStore.h"

#include <ctype.h>
#include <esp_system.h>
#include <mbedtls/md.h>

namespace {
constexpr uint32_t kRegistryVersion = 1;
constexpr uint32_t kPasswordIterations = 12000;
constexpr uint32_t kSessionLifetimeMs = 60UL * 60UL * 1000UL;
constexpr char kRegistryKey[] = "registry";

bool constantTimeEqual(const uint8_t* left, const uint8_t* right, size_t size) {
  uint8_t difference = 0;
  for (size_t index = 0; index < size; ++index) {
    difference |= left[index] ^ right[index];
  }
  return difference == 0;
}

bool terminated(const char* value, size_t capacity) {
  return memchr(value, '\0', capacity) != nullptr;
}

bool validRecord(const SensorRecord& sensor) {
  return sensor.id[0] != '\0' && terminated(sensor.id, sizeof(sensor.id)) &&
         terminated(sensor.type, sizeof(sensor.type)) &&
         terminated(sensor.name, sizeof(sensor.name)) &&
         terminated(sensor.description, sizeof(sensor.description)) &&
         terminated(sensor.grouping, sizeof(sensor.grouping)) &&
         terminated(sensor.location, sizeof(sensor.location)) &&
         terminated(sensor.primaryLabel, sizeof(sensor.primaryLabel)) &&
         terminated(sensor.primaryValue, sizeof(sensor.primaryValue));
}
}  // namespace

void HomeHubStore::begin() {
  ready_ = preferences_.begin("homehub", false);
  if (!ready_) return;

  accountReady_ = preferences_.getBool("account", false);
  if (accountReady_) {
    preferences_.getBytes("salt", salt_, sizeof(salt_));
    preferences_.getBytes("passhash", passwordHash_, sizeof(passwordHash_));
    String username = preferences_.getString("username", "");
    if (username.length() == 0 || username.length() >= sizeof(username_)) {
      accountReady_ = false;
    } else {
      strlcpy(username_, username.c_str(), sizeof(username_));
    }
  }
  registryReady_ = loadRegistry();
}

bool HomeHubStore::derivePassword(const String& password, const uint8_t* salt,
                                 uint8_t* output) const {
  const mbedtls_md_info_t* md = mbedtls_md_info_from_type(MBEDTLS_MD_SHA256);
  if (md == nullptr || password.isEmpty()) return false;

  uint8_t blockInput[20];
  memcpy(blockInput, salt, 16);
  blockInput[16] = 0;
  blockInput[17] = 0;
  blockInput[18] = 0;
  blockInput[19] = 1;

  uint8_t u[32];
  uint8_t nextU[32];
  uint8_t accumulator[32];
  if (mbedtls_md_hmac(md, reinterpret_cast<const uint8_t*>(password.c_str()),
                      password.length(), blockInput, sizeof(blockInput), u) != 0) {
    return false;
  }
  memcpy(accumulator, u, sizeof(accumulator));
  for (uint32_t iteration = 1; iteration < kPasswordIterations; ++iteration) {
    if (mbedtls_md_hmac(md, reinterpret_cast<const uint8_t*>(password.c_str()),
                        password.length(), u, sizeof(u), nextU) != 0) {
      return false;
    }
    for (size_t byte = 0; byte < sizeof(accumulator); ++byte) {
      accumulator[byte] ^= nextU[byte];
    }
    memcpy(u, nextU, sizeof(u));
  }
  memcpy(output, accumulator, sizeof(accumulator));
  memset(u, 0, sizeof(u));
  memset(nextU, 0, sizeof(nextU));
  memset(accumulator, 0, sizeof(accumulator));
  return true;
}

bool HomeHubStore::createAccount(const String& username, const String& password) {
  if (!ready_ || accountReady_ || username.length() < 3 || username.length() > 24 ||
      password.length() < 12 || password.length() > 64) {
    return false;
  }
  for (size_t index = 0; index < username.length(); ++index) {
    const char value = username[index];
    if (!isalnum(static_cast<unsigned char>(value)) && value != '_' && value != '-') {
      return false;
    }
  }

  uint8_t salt[sizeof(salt_)];
  uint8_t passwordHash[sizeof(passwordHash_)];
  esp_fill_random(salt, sizeof(salt));
  if (!derivePassword(password, salt, passwordHash)) return false;

  const bool saved = preferences_.putString("username", username) > 0 &&
                     preferences_.putBytes("salt", salt, sizeof(salt)) == sizeof(salt) &&
                     preferences_.putBytes("passhash", passwordHash,
                                           sizeof(passwordHash)) == sizeof(passwordHash) &&
                     preferences_.putBool("account", true);
  if (!saved) return false;

  strlcpy(username_, username.c_str(), sizeof(username_));
  memcpy(salt_, salt, sizeof(salt_));
  memcpy(passwordHash_, passwordHash, sizeof(passwordHash_));
  accountReady_ = true;
  memset(passwordHash, 0, sizeof(passwordHash));
  return true;
}

bool HomeHubStore::verifyAccount(const String& username, const String& password) {
  if (!ready_ || !accountReady_ || username != username_ || password.length() > 64) {
    return false;
  }
  uint8_t candidate[sizeof(passwordHash_)];
  const bool derived = derivePassword(password, salt_, candidate);
  const bool matches = derived && constantTimeEqual(candidate, passwordHash_, sizeof(candidate));
  memset(candidate, 0, sizeof(candidate));
  return matches;
}

String HomeHubStore::startSession() {
  uint8_t randomBytes[16];
  esp_fill_random(randomBytes, sizeof(randomBytes));
  static constexpr char hex[] = "0123456789abcdef";
  for (size_t index = 0; index < sizeof(randomBytes); ++index) {
    session_[index * 2] = hex[randomBytes[index] >> 4];
    session_[index * 2 + 1] = hex[randomBytes[index] & 0x0f];
  }
  session_[32] = '\0';
  sessionStartedMs_ = millis();
  return String(session_);
}

bool HomeHubStore::isSessionValid(const String& token) const {
  return session_[0] != '\0' && token.length() == 32 && token == session_ &&
         millis() - sessionStartedMs_ < kSessionLifetimeMs;
}

void HomeHubStore::endSession() {
  memset(session_, 0, sizeof(session_));
  sessionStartedMs_ = 0;
}

void HomeHubStore::expireSession() {
  if (session_[0] != '\0' && millis() - sessionStartedMs_ >= kSessionLifetimeMs) {
    endSession();
  }
}

const SensorRecord* HomeHubStore::sensorAt(size_t index) const {
  return index < sensorCount_ ? &sensors_[index] : nullptr;
}

bool HomeHubStore::upsertSensor(const SensorRecord& sensor, bool persist) {
  if (!registryReady_ || !validRecord(sensor)) {
    return false;
  }
  size_t index = 0;
  while (index < sensorCount_ && strcmp(sensors_[index].id, sensor.id) != 0) ++index;
  if (index == sensorCount_) {
    if (sensorCount_ >= kHomeHubMaximumSensors) return false;
    ++sensorCount_;
  }
  sensors_[index] = sensor;
  return !persist || saveRegistry();
}

SensorRecord* HomeHubStore::findSensor(const char* id) {
  if (id == nullptr) return nullptr;
  for (size_t index = 0; index < sensorCount_; ++index) {
    if (strcmp(sensors_[index].id, id) == 0) return &sensors_[index];
  }
  return nullptr;
}

bool HomeHubStore::setSensorFlag(const char* id, uint8_t flag, bool enabled) {
  SensorRecord* sensor = findSensor(id);
  if (sensor == nullptr) return false;
  if (enabled) sensor->flags |= flag;
  else sensor->flags &= static_cast<uint8_t>(~flag);
  return saveRegistry();
}

bool HomeHubStore::updateLiveValue(const char* id, const char* label,
                                   const char* value, uint32_t receivedMs,
                                   uint16_t radioAddress) {
  SensorRecord* sensor = findSensor(id);
  if (sensor == nullptr || label == nullptr || value == nullptr ||
      strnlen(label, sizeof(sensor->primaryLabel)) >= sizeof(sensor->primaryLabel) ||
      strnlen(value, sizeof(sensor->primaryValue)) >= sizeof(sensor->primaryValue)) {
    return false;
  }
  strlcpy(sensor->primaryLabel, label, sizeof(sensor->primaryLabel));
  strlcpy(sensor->primaryValue, value, sizeof(sensor->primaryValue));
  sensor->lastSeenMs = receivedMs;
  sensor->radioAddress = radioAddress;
  markSensorReported(*sensor);
  return true;
}

bool HomeHubStore::saveRegistry() {
  struct RegistryHeader {
    uint32_t version;
    uint32_t count;
  } header{kRegistryVersion, static_cast<uint32_t>(sensorCount_)};
  const size_t bytes = sizeof(header) + sensorCount_ * sizeof(SensorRecord);
  uint8_t* buffer = static_cast<uint8_t*>(malloc(bytes));
  if (buffer == nullptr) return false;
  memcpy(buffer, &header, sizeof(header));
  memcpy(buffer + sizeof(header), sensors_, sensorCount_ * sizeof(SensorRecord));
  const bool saved = preferences_.putBytes(kRegistryKey, buffer, bytes) == bytes;
  free(buffer);
  registryReady_ = saved;
  return saved;
}

bool HomeHubStore::saveRouterCredentials(const String& ssid, const String& password) {
  if (!ready_ || ssid.length() == 0 || ssid.length() > 32 || password.length() > 64) return false;
  return preferences_.putString("router_ssid", ssid) > 0 &&
         preferences_.putString("router_pass", password) > 0;
}

bool HomeHubStore::loadRouterCredentials(String& ssid, String& password) {
  if (!ready_ || !preferences_.isKey("router_ssid")) return false;
  ssid = preferences_.getString("router_ssid", "");
  password = preferences_.getString("router_pass", "");
  return ssid.length() > 0 && ssid.length() <= 32 && password.length() <= 64;
}

bool HomeHubStore::loadRegistry() {
  const size_t bytes = preferences_.getBytesLength(kRegistryKey);
  struct RegistryHeader {
    uint32_t version;
    uint32_t count;
  } header{};
  if (bytes == 0) return true;
  if (bytes < sizeof(header) || bytes > sizeof(header) +
                                       kHomeHubMaximumSensors * sizeof(SensorRecord)) {
    return false;
  }
  uint8_t* buffer = static_cast<uint8_t*>(malloc(bytes));
  if (buffer == nullptr) return false;
  if (preferences_.getBytes(kRegistryKey, buffer, bytes) != bytes) {
    free(buffer);
    return false;
  }
  memcpy(&header, buffer, sizeof(header));
  if (header.version != kRegistryVersion || header.count > kHomeHubMaximumSensors ||
      bytes != sizeof(header) + header.count * sizeof(SensorRecord)) {
    free(buffer);
    return false;
  }
  sensorCount_ = header.count;
  memcpy(sensors_, buffer + sizeof(header), sensorCount_ * sizeof(SensorRecord));
  free(buffer);
  for (size_t index = 0; index < sensorCount_; ++index) {
    if (!validRecord(sensors_[index])) return false;
    sensors_[index].lastSeenMs = 0;
  }
  return true;
}
