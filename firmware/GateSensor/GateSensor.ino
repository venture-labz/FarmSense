// Farm Sense Gate Sensor 01.
// Target: Seeed Studio XIAO ESP32-C3.
// Hall OUT -> D1 (GPIO3), Hall VCC -> 3V3, Hall GND -> GND.
// RYLR998: D6 -> RXD, D7 <- TXD.
//
// This sketch defaults to diagnostic mode so magnet behavior can be observed
// safely over USB and LoRa. Set kDiagnosticMode to false only after CLOSED has
// been calibrated from the Home Hub. Production mode uses Hall-level and timer
// wakeups, a 60-minute default ALIVE interval, and RYLR998 sleep mode.

#include <Arduino.h>
#include <Preferences.h>
#include <esp_sleep.h>

namespace {
constexpr bool kDiagnosticMode = true;
constexpr char kSensorId[] = "gate-01";
constexpr uint8_t kHallPin = D1;
constexpr uint8_t kLoraTxPin = D6;
constexpr uint8_t kLoraRxPin = D7;
constexpr uint32_t kLoraBaudRate = 115'200;
constexpr uint32_t kSampleIntervalMs = 50;
constexpr uint8_t kStableSampleCount = 3;
constexpr uint8_t kDiagnosticStableSampleCount = 1;
constexpr uint32_t kConfigListenMs = 5'000;
constexpr uint32_t kDefaultAliveSeconds = 3'600;
constexpr uint32_t kDiagnosticAliveSeconds = 10;
constexpr uint32_t kMinimumAliveSeconds = 5;
constexpr uint32_t kMaximumAliveSeconds = 86'400;
constexpr uint32_t kEventReceiptTimeoutMs = 2'000;
constexpr uint8_t kMaximumEventAttempts = 3;

Preferences preferences;
String loraLine;
int candidateRaw = -1;
uint8_t candidateCount = 0;
int stableRaw = -1;
uint32_t lastSampleMs = 0;
uint32_t lastDiagnosticAliveMs = 0;
uint32_t sequence = 0;
int closedRaw = -1;
bool alarmEnabled = true;
uint32_t aliveSeconds = kDefaultAliveSeconds;
uint32_t configRevision = 0;
bool eventAwaitingReceipt = false;
uint32_t pendingEventSequence = 0;
int pendingEventRaw = -1;
uint8_t pendingEventAttempts = 0;
uint32_t lastEventSendMs = 0;

RTC_DATA_ATTR int previousSleepRaw = -1;

String gateStateForRaw(const int raw) {
  if (closedRaw < 0) {
    return "UNCONFIGURED";
  }
  return raw == closedRaw ? "CLOSED" : "OPEN";
}

void pollLora();

void sendPacket(const char* kind, const int raw, const uint32_t packetSequence) {
  const String payload = String("FS2|GATE|") + kSensorId + "|" + String(packetSequence) +
                         "|" + kind + "|" + String(raw) + "|" + gateStateForRaw(raw) +
                         "|" + String(millis());
  Serial1.print("AT+SEND=0,");  // Test-node broadcast; application ID filters payloads.
  Serial1.print(payload.length());
  Serial1.print(',');
  Serial1.print(payload);
  Serial1.print("\r\n");
  if (String(kind) == "EVENT") {
    Serial.print("LoRa EVENT queued: ");
    Serial.println(payload);
  }
}

void sendAlive(const int raw) {
  sendPacket("ALIVE", raw, ++sequence);
}

void beginEvent(const int raw) {
  // One state change produces one immediate LoRa event. Reliability retries are
  // intentionally disabled during timing diagnostics.
  sendPacket("EVENT", raw, ++sequence);
}

void retryPendingEvent() {
  if (!eventAwaitingReceipt || millis() - lastEventSendMs < kEventReceiptTimeoutMs) {
    return;
  }
  if (pendingEventAttempts >= kMaximumEventAttempts) {
    Serial.printf("LoRa EVENT %lu unconfirmed after %u attempts\n",
                  static_cast<unsigned long>(pendingEventSequence), pendingEventAttempts);
    eventAwaitingReceipt = false;
    return;
  }
  ++pendingEventAttempts;
  lastEventSendMs = millis();
  Serial.printf("Retrying LoRa EVENT %lu (%u/%u)\n",
                static_cast<unsigned long>(pendingEventSequence), pendingEventAttempts,
                kMaximumEventAttempts);
  sendPacket("EVENT", pendingEventRaw, pendingEventSequence);
}

void saveConfiguration() {
  preferences.putInt("closed_raw", closedRaw);
  preferences.putBool("alarm", alarmEnabled);
  preferences.putUInt("alive_sec", aliveSeconds);
  preferences.putUInt("cfg_rev", configRevision);
}

bool applyConfiguration(const String& payload) {
  // FS2|CFG|gate-01|revision|closed_raw|alarm_enabled|alive_seconds
  String values[7];
  int valueIndex = 0;
  int start = 0;
  while (valueIndex < 7) {
    const int separator = payload.indexOf('|', start);
    values[valueIndex++] = separator < 0 ? payload.substring(start)
                                         : payload.substring(start, separator);
    if (separator < 0) {
      break;
    }
    start = separator + 1;
  }
  if (valueIndex != 7 || values[0] != "FS2" || values[1] != "CFG" ||
      values[2] != kSensorId) {
    return false;
  }

  const uint32_t receivedRevision = values[3].toInt();
  const int receivedClosedRaw = values[4].toInt();
  const uint32_t receivedAliveSeconds = values[6].toInt();
  if (receivedRevision == 0 || receivedClosedRaw < 0 || receivedClosedRaw > 1 ||
      receivedAliveSeconds < kMinimumAliveSeconds ||
      receivedAliveSeconds > kMaximumAliveSeconds) {
    Serial.println("Rejected invalid configuration packet");
    return false;
  }

  closedRaw = receivedClosedRaw;
  alarmEnabled = values[5] == "1";
  aliveSeconds = receivedAliveSeconds;
  configRevision = receivedRevision;
  saveConfiguration();

  const String acknowledgement = String("FS2|ACK|") + kSensorId + "|" +
                                 String(configRevision) + "|OK";
  Serial1.print("AT+SEND=0,");
  Serial1.print(acknowledgement.length());
  Serial1.print(',');
  Serial1.print(acknowledgement);
  Serial1.print("\r\n");
  Serial.print("Applied configuration: ");
  Serial.println(acknowledgement);
  return true;
}

bool applyEventReceipt(const String& payload) {
  // FS2|RCPT|gate-01|event_sequence|OK
  String values[5];
  int valueIndex = 0;
  int start = 0;
  while (valueIndex < 5) {
    const int separator = payload.indexOf('|', start);
    values[valueIndex++] = separator < 0 ? payload.substring(start)
                                         : payload.substring(start, separator);
    if (separator < 0) break;
    start = separator + 1;
  }
  if (valueIndex != 5 || values[0] != "FS2" || values[1] != "RCPT" ||
      values[2] != kSensorId || values[4] != "OK") {
    return false;
  }
  const uint32_t receivedSequence = values[3].toInt();
  if (eventAwaitingReceipt && receivedSequence == pendingEventSequence) {
    eventAwaitingReceipt = false;
    Serial.printf("LoRa EVENT %lu received by Home Hub\n",
                  static_cast<unsigned long>(receivedSequence));
  }
  return true;
}

bool applyClockSync(const String& payload) {
  // FS2|SYNC|gate-01|hub_sent_ms
  String values[4];
  int count = 0, start = 0;
  while (count < 4) {
    const int separator = payload.indexOf('|', start);
    values[count++] = separator < 0 ? payload.substring(start) : payload.substring(start, separator);
    if (separator < 0) break;
    start = separator + 1;
  }
  if (count != 4 || values[0] != "FS2" || values[1] != "SYNC" || values[2] != kSensorId) return false;
  const String reply = String("FS2|SYNC|") + kSensorId + "|" + values[3] + "|" + String(millis());
  Serial1.print("AT+SEND=0,");
  Serial1.print(reply.length());
  Serial1.print(',');
  Serial1.print(reply);
  Serial1.print("\r\n");
  return true;
}

void processLoraLine(const String& line) {
  Serial.print("RYLR998: ");
  Serial.println(line);
  if (!line.startsWith("+RCV=")) {
    return;
  }
  const int firstComma = line.indexOf(',');
  const int secondComma = line.indexOf(',', firstComma + 1);
  const int thirdComma = line.indexOf(',', secondComma + 1);
  if (firstComma < 0 || secondComma < 0 || thirdComma < 0) {
    return;
  }
  const String payload = line.substring(secondComma + 1, thirdComma);
  if (!applyConfiguration(payload) && !applyEventReceipt(payload)) applyClockSync(payload);
}

void pollLora() {
  while (Serial1.available()) {
    const char character = static_cast<char>(Serial1.read());
    if (character == '\r') {
      continue;
    }
    if (character == '\n') {
      if (!loraLine.isEmpty()) {
        processLoraLine(loraLine);
        loraLine = "";
      }
      continue;
    }
    if (loraLine.length() < 240) {
      loraLine += character;
    } else {
      loraLine = "";
    }
  }
}

int readStableRaw() {
  int observed = digitalRead(kHallPin) == HIGH ? 1 : 0;
  for (uint8_t count = 1; count < kStableSampleCount; ++count) {
    delay(kSampleIntervalMs);
    const int next = digitalRead(kHallPin) == HIGH ? 1 : 0;
    if (next != observed) {
      observed = next;
      count = 0;
    }
  }
  return observed;
}

void sampleDiagnosticHall() {
  const uint32_t nowMs = millis();
  if (nowMs - lastSampleMs < kSampleIntervalMs) {
    return;
  }
  lastSampleMs = nowMs;
  const int raw = digitalRead(kHallPin) == HIGH ? 1 : 0;
  if (raw != candidateRaw) {
    candidateRaw = raw;
    candidateCount = 1;
    return;
  }
  if (candidateCount < kDiagnosticStableSampleCount) {
    ++candidateCount;
  }
  if (candidateCount == kDiagnosticStableSampleCount && raw != stableRaw) {
    stableRaw = raw;
    Serial.printf("Hall stable raw=%d state=%s\n", stableRaw,
                  gateStateForRaw(stableRaw).c_str());
    beginEvent(stableRaw);
  }
}

void enterProductionSleep(const int raw) {
  Serial1.print("AT+MODE=1\r\n");  // Volatile RYLR998 low-power mode.
  delay(100);
  const esp_deepsleep_gpio_wake_up_mode_t nextLevel =
      raw == LOW ? ESP_GPIO_WAKEUP_GPIO_HIGH : ESP_GPIO_WAKEUP_GPIO_LOW;
  esp_deep_sleep_enable_gpio_wakeup(1ULL << kHallPin, nextLevel);
  esp_sleep_enable_timer_wakeup(static_cast<uint64_t>(aliveSeconds) * 1'000'000ULL);
  Serial.flush();
  esp_deep_sleep_start();
}
}  // namespace

void setup() {
  Serial.begin(115200);
  pinMode(kHallPin, INPUT);
  preferences.begin("gate-sensor", false);
  closedRaw = preferences.getInt("closed_raw", -1);
  alarmEnabled = preferences.getBool("alarm", true);
  aliveSeconds = preferences.getUInt("alive_sec", kDefaultAliveSeconds);
  configRevision = preferences.getUInt("cfg_rev", 0);
  if (aliveSeconds < kMinimumAliveSeconds || aliveSeconds > kMaximumAliveSeconds) {
    aliveSeconds = kDefaultAliveSeconds;
  }

  Serial1.begin(kLoraBaudRate, SERIAL_8N1, kLoraRxPin, kLoraTxPin);
  delay(250);
  Serial.println();
  Serial.println(kDiagnosticMode ? "Farm Sense Gate Sensor diagnostic" :
                                  "Farm Sense Gate Sensor low-power mode");

  if (!kDiagnosticMode) {
    stableRaw = readStableRaw();
    const bool changed = previousSleepRaw >= 0 && stableRaw != previousSleepRaw;
    if (changed) {
      beginEvent(stableRaw);
    } else {
      sendAlive(stableRaw);
    }
    previousSleepRaw = stableRaw;
    const uint32_t untilMs = millis() + kConfigListenMs;
    while (static_cast<int32_t>(untilMs - millis()) > 0) {
      pollLora();
    }
    enterProductionSleep(stableRaw);
  }
}

void loop() {
  if (kDiagnosticMode) {
    pollLora();
    sampleDiagnosticHall();
    const uint32_t diagnosticIntervalMs =
        static_cast<uint32_t>(configRevision == 0 ? kDiagnosticAliveSeconds : aliveSeconds) *
        1'000UL;
    if (millis() - lastDiagnosticAliveMs >= diagnosticIntervalMs && stableRaw >= 0) {
      lastDiagnosticAliveMs = millis();
      sendAlive(stableRaw);
    }
  }
}
