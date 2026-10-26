#pragma once
#include <stdint.h>

namespace HomeHubEnvironment {
struct Reading {
  bool ready = false;
  float temperatureC = 0;
  float pressureHpa = 0;
  uint32_t lastSeenMs = 0;
};
void begin();
void poll();
const Reading& reading();
}
