#pragma once

#include <WebServer.h>

#include "HomeHubRadio.h"
#include "HomeHubStore.h"

class HomeHubWeb {
 public:
  HomeHubWeb(WebServer& server, HomeHubStore& store, HomeHubRadio& radio);
  void begin();
  void expireSession();
  void startWifiAttempt();

 private:
  bool requireSession();
  String requestCookie(const char* name) const;
  void serveRoot();
  void serveAuthState();
  void createAccount();
  void login();
  void logout();
  void serveSensors();
  void serveEvents();
  void serveNetworkState();
  void waitForSensor();
  void sensorAction();
  void editSensor();
  void configureWifi();
  void noStore();

  WebServer& server_;
  HomeHubStore& store_;
  HomeHubRadio& radio_;
  bool wifiAttempted_ = false;
  uint32_t wifiStartedMs_ = 0;
};
