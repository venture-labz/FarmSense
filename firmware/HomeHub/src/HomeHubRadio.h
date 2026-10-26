#pragma once

#include <Arduino.h>

#include "HomeHubStore.h"

class HomeHubRadio {
 public:
  explicit HomeHubRadio(HomeHubStore& store) : store_(store) {}
  void begin();
  void poll();
  void sendSetupStatus(const SensorRecord& sensor);

 private:
  void processLine(char* line);
  void processPayload(char* payload, uint16_t radioAddress);
  bool sendPayload(uint16_t radioAddress, const String& payload);

  HomeHubStore& store_;
  char line_[300]{};
  size_t lineLength_ = 0;
  bool lineOverflow_ = false;
};
