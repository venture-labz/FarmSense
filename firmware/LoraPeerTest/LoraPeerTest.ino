// Farm Sense periodic RYLR998 peer test transmitter.
// Target: Seeed Studio XIAO ESP32-C3; D6 -> RYLR998 RXD, D7 <- RYLR998 TXD.
// Sends a broadcast packet at startup and every 60 seconds using the module's
// existing stored configuration.
// It deliberately does not issue any RYLR998 configuration command.

#include <Arduino.h>

namespace {
constexpr uint8_t kLoraTxPin = D6;
constexpr uint8_t kLoraRxPin = D7;
constexpr uint32_t kLoraBaudRate = 115'200;
constexpr uint32_t kHealthCheckDelayMs = 500;
constexpr uint32_t kHealthRetryMs = 15'000;
constexpr uint32_t kSendIntervalMs = 60'000;

bool loraHealthy = false;
uint32_t lastHealthCheckMs = 0;
uint32_t lastSendMs = 0;
uint32_t sequence = 0;
String loraLine;

void printLoraResponse(const String& line) {
  Serial.print("RYLR998: ");
  Serial.println(line);
  if (line == "+OK") {
    loraHealthy = true;
    Serial.println("RYLR998: UART health check passed");
  }
}

void pollLora() {
  while (Serial1.available()) {
    const char character = static_cast<char>(Serial1.read());
    if (character == '\r') {
      continue;
    }
    if (character == '\n') {
      if (!loraLine.isEmpty()) {
        printLoraResponse(loraLine);
        loraLine = "";
      }
      continue;
    }
    if (loraLine.length() < 240) {
      loraLine += character;
    } else {
      loraLine = "";
      Serial.println("RYLR998: discarded overlong UART line");
    }
  }
}

void requestLoraHealth() {
  Serial1.print("AT\r\n");
  lastHealthCheckMs = millis();
  Serial.println("RYLR998: sent AT health check");
}

void sendBroadcastTest() {
  ++sequence;
  const String payload = "FARMSENSE-LORA-TEST-" + String(sequence);
  Serial1.print("AT+SEND=0,");  // Address zero is the documented broadcast address.
  Serial1.print(payload.length());
  Serial1.print(',');
  Serial1.print(payload);
  Serial1.print("\r\n");
  lastSendMs = millis();
  Serial.print("RYLR998: sent broadcast test packet #");
  Serial.println(sequence);
}
}  // namespace

void setup() {
  Serial.begin(115200);
  Serial1.begin(kLoraBaudRate, SERIAL_8N1, kLoraRxPin, kLoraTxPin);
  delay(kHealthCheckDelayMs);
  Serial.println();
  Serial.println("Farm Sense LoRa peer test");
  requestLoraHealth();
}

void loop() {
  pollLora();
  const uint32_t nowMs = millis();
  if (!loraHealthy && nowMs - lastHealthCheckMs >= kHealthRetryMs) {
    requestLoraHealth();
  }
  if (loraHealthy && (sequence == 0 || nowMs - lastSendMs >= kSendIntervalMs)) {
    sendBroadcastTest();
  }
}
