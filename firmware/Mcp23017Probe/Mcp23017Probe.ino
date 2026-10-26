// Farm Sense MCP23017 presence probe.
// Target: Seeed Studio XIAO ESP32-C3; I2C SCL=D4, SDA=D5.
// This sketch only performs I2C address probes. It does not configure or drive
// any MCP23017 pins.

#include <Arduino.h>
#include <Wire.h>

namespace {
constexpr uint8_t kSclPin = D4;
constexpr uint8_t kSdaPin = D5;
constexpr uint8_t kExpectedMcp23017Address = 0x20;
constexpr uint32_t kScanIntervalMs = 5'000;

void printAddress(const uint8_t address) {
  if (address < 0x10) {
    Serial.print('0');
  }
  Serial.print(address, HEX);
}

void scanBus() {
  bool foundExpectedAddress = false;
  uint8_t deviceCount = 0;

  Serial.println("I2C scan: begin");
  for (uint8_t address = 0x03; address <= 0x77; ++address) {
    Wire.beginTransmission(address);
    const uint8_t result = Wire.endTransmission();
    if (result != 0) {
      continue;
    }

    ++deviceCount;
    Serial.print("  device at 0x");
    printAddress(address);
    if (address == kExpectedMcp23017Address) {
      foundExpectedAddress = true;
      Serial.print(" (expected MCP23017)");
    }
    Serial.println();
  }

  Serial.print("I2C scan: ");
  Serial.print(deviceCount);
  Serial.println(deviceCount == 1 ? " device" : " devices");
  Serial.print("MCP23017 at 0x20: ");
  Serial.println(foundExpectedAddress ? "DETECTED" : "NOT DETECTED");
}
}  // namespace

void setup() {
  Serial.begin(115200);
  Wire.begin(kSdaPin, kSclPin);
  Wire.setTimeOut(50);
  delay(200);
  Serial.println();
  Serial.println("Farm Sense MCP23017 I2C probe");
}

void loop() {
  static uint32_t lastScanMs = UINT32_MAX - kScanIntervalMs;
  const uint32_t nowMs = millis();
  if (nowMs - lastScanMs < kScanIntervalMs) {
    return;
  }

  lastScanMs = nowMs;
  scanBus();
}
