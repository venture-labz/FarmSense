#include "HomeHubEnvironment.h"
#include <Arduino.h>
#include <Wire.h>
#include <math.h>

namespace {
constexpr uint8_t kI2cSclPin = D4;
constexpr uint8_t kI2cSdaPin = D5;
constexpr uint8_t kBmp280Address = 0x76;
constexpr uint8_t kBmp280ChipId = 0x58;
constexpr uint8_t kBme280ChipId = 0x60;
struct BmpCalibration {
  uint16_t t1; int16_t t2, t3; uint16_t p1;
  int16_t p2, p3, p4, p5, p6, p7, p8, p9;
} bmpCalibration{};
HomeHubEnvironment::Reading current;
bool initialized = false;
uint32_t lastAttemptMs = 0;
uint16_t readU16LE(const uint8_t* bytes) {
  return static_cast<uint16_t>(bytes[0]) | (static_cast<uint16_t>(bytes[1]) << 8);
}
int16_t readS16LE(const uint8_t* bytes) { return static_cast<int16_t>(readU16LE(bytes)); }
bool readRegisters(uint8_t address, uint8_t first, uint8_t* values, size_t count) {
  Wire.beginTransmission(address);
  Wire.write(first);
  if (Wire.endTransmission(false) != 0) return false;
  if (Wire.requestFrom(static_cast<int>(address), static_cast<int>(count)) != count) {
    while (Wire.available()) Wire.read();
    return false;
  }
  for (size_t i = 0; i < count; ++i) values[i] = Wire.read();
  return true;
}

// Ported from the hardware-validated PeripheralValidation target.
bool initializeBmp280() {
  uint8_t chipId = 0;
  if (!readRegisters(kBmp280Address, 0xD0, &chipId, 1)) {
    return false;
  }
  if (chipId == kBme280ChipId) {
    return false;
  }
  if (chipId != kBmp280ChipId) {
    return false;
  }

  uint8_t calibration[24]{};
  if (!readRegisters(kBmp280Address, 0x88, calibration, sizeof(calibration))) {
    return false;
  }

  bmpCalibration = {
      readU16LE(&calibration[0]),  readS16LE(&calibration[2]),
      readS16LE(&calibration[4]),  readU16LE(&calibration[6]),
      readS16LE(&calibration[8]),  readS16LE(&calibration[10]),
      readS16LE(&calibration[12]), readS16LE(&calibration[14]),
      readS16LE(&calibration[16]), readS16LE(&calibration[18]),
      readS16LE(&calibration[20]), readS16LE(&calibration[22]),
  };
  if (bmpCalibration.t1 == 0 || bmpCalibration.p1 == 0) {
    return false;
  }

  // Temperature and pressure x1 oversampling, normal mode. This setting is
  // volatile and is required to produce fresh measurements.
  Wire.beginTransmission(kBmp280Address);
  Wire.write(0xF4);
  Wire.write(0x27);
  if (Wire.endTransmission() != 0) {
    return false;
  }
  return true;
}

bool readBmp280(float* temperatureC, float* pressureHpa) {
  uint8_t raw[6]{};
  if (!readRegisters(kBmp280Address, 0xF7, raw, sizeof(raw))) {
    return false;
  }

  const int32_t adcPressure = (static_cast<int32_t>(raw[0]) << 12) |
                              (static_cast<int32_t>(raw[1]) << 4) |
                              (raw[2] >> 4);
  const int32_t adcTemperature = (static_cast<int32_t>(raw[3]) << 12) |
                                 (static_cast<int32_t>(raw[4]) << 4) |
                                 (raw[5] >> 4);
  if (adcPressure == 0x80000 || adcTemperature == 0x80000) {
    return false;
  }

  int32_t variable1 = (((adcTemperature >> 3) -
                        (static_cast<int32_t>(bmpCalibration.t1) << 1)) *
                       bmpCalibration.t2) >>
                      11;
  int32_t variable2 = (((((adcTemperature >> 4) - bmpCalibration.t1) *
                         ((adcTemperature >> 4) - bmpCalibration.t1)) >>
                        12) *
                       bmpCalibration.t3) >>
                      14;
  const int32_t fineTemperature = variable1 + variable2;
  *temperatureC = static_cast<float>((fineTemperature * 5 + 128) >> 8) / 100.0F;

  int64_t pressureVariable1 = static_cast<int64_t>(fineTemperature) - 128000;
  int64_t pressureVariable2 = pressureVariable1 * pressureVariable1 * bmpCalibration.p6;
  pressureVariable2 += (pressureVariable1 * bmpCalibration.p5) * 131072LL;
  pressureVariable2 += static_cast<int64_t>(bmpCalibration.p4) * 34359738368LL;
  pressureVariable1 = ((pressureVariable1 * pressureVariable1 * bmpCalibration.p3) >> 8) +
                      ((pressureVariable1 * bmpCalibration.p2) * 4096LL);
  pressureVariable1 =
      (((static_cast<int64_t>(1) << 47) + pressureVariable1) * bmpCalibration.p1) >> 33;
  if (pressureVariable1 == 0) {
    return false;
  }

  int64_t pressure = 1048576 - adcPressure;
  pressure = (((pressure << 31) - pressureVariable2) * 3125) / pressureVariable1;
  pressureVariable1 = (bmpCalibration.p9 * (pressure >> 13) * (pressure >> 13)) >> 25;
  pressureVariable2 = (bmpCalibration.p8 * pressure) >> 19;
  pressure = ((pressure + pressureVariable1 + pressureVariable2) >> 8) +
             (static_cast<int64_t>(bmpCalibration.p7) * 16LL);
  *pressureHpa = static_cast<float>(pressure) / 25600.0F;
  return true;
}


}
namespace HomeHubEnvironment {
void begin() {
  Wire.begin(kI2cSdaPin, kI2cSclPin);
  Wire.setTimeOut(50);
  initialized = initializeBmp280();
  current.ready = false;
  lastAttemptMs = millis();
}
void poll() {
  const uint32_t now = millis();
  if (now - lastAttemptMs < (initialized ? 2000U : 10000U)) return;
  lastAttemptMs = now;
  if (!initialized) {
    initialized = initializeBmp280();
    return; // Allow a measurement cycle before reading.
  }
  float temperature = 0, pressure = 0;
  if (!readBmp280(&temperature, &pressure) || !isfinite(temperature) ||
      !isfinite(pressure) || temperature < -40 || temperature > 85 ||
      pressure < 300 || pressure > 1100) {
    current.ready = false;
    initialized = false;
    return;
  }
  current.temperatureC = temperature;
  current.pressureHpa = pressure;
  current.lastSeenMs = now;
  current.ready = true;
}
const Reading& reading() { return current; }
}
