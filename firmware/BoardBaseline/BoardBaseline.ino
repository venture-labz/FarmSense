// Farm Sense board baseline: D1 indicator slow fade.
// Target: Seeed Studio XIAO ESP32-C3.

#include <Arduino.h>

namespace {
constexpr uint8_t kIndicatorPin = D1;
constexpr uint32_t kPwmFrequencyHz = 5'000;
constexpr uint8_t kPwmResolutionBits = 8;
constexpr uint32_t kFadeHalfCycleMs = 2'000;
constexpr uint32_t kFadeCycleMs = kFadeHalfCycleMs * 2;
constexpr uint32_t kUpdateIntervalMs = 10;
constexpr uint32_t kMaxDuty = (1UL << kPwmResolutionBits) - 1;

uint32_t lastUpdateMs = 0;

uint32_t fadeDuty(const uint32_t nowMs) {
  const uint32_t positionMs = nowMs % kFadeCycleMs;
  if (positionMs < kFadeHalfCycleMs) {
    return map(positionMs, 0, kFadeHalfCycleMs, 0, kMaxDuty);
  }
  return map(positionMs, kFadeHalfCycleMs, kFadeCycleMs, kMaxDuty, 0);
}
}  // namespace

void setup() {
  ledcAttach(kIndicatorPin, kPwmFrequencyHz, kPwmResolutionBits);
  ledcWrite(kIndicatorPin, 0);
}

void loop() {
  const uint32_t nowMs = millis();
  if (nowMs - lastUpdateMs < kUpdateIntervalMs) {
    return;
  }

  lastUpdateMs = nowMs;
  ledcWrite(kIndicatorPin, fadeDuty(nowMs));
}
