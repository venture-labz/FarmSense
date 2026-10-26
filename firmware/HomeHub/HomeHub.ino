#include <Arduino.h>
#include <WebServer.h>
#include <WiFi.h>

#include "src/HomeHubStore.h"
#include "src/HomeHubRadio.h"
#include "src/HomeHubWeb.h"
#include "src/HomeHubEnvironment.h"

namespace {
constexpr char kAccessPointSsid[] = "FarmSense-HomeHub";
const IPAddress kHomeHubIp(192, 168, 4, 1);

WebServer server(80);
HomeHubStore store;
HomeHubRadio radio(store);
HomeHubWeb web(server, store, radio);
}  // namespace

void setup() {
  Serial.begin(115200);
  store.begin();
  radio.begin();
  HomeHubEnvironment::begin();

  WiFi.mode(WIFI_AP_STA);
  WiFi.setSleep(false);
  if (!WiFi.softAPConfig(kHomeHubIp, kHomeHubIp, IPAddress(255, 255, 255, 0)) ||
      !WiFi.softAP(kAccessPointSsid, nullptr, 1, false, 4)) {
    Serial.println("Home Hub Wi-Fi failed to start");
  }
  String routerSsid;
  String routerPassword;
  if (store.loadRouterCredentials(routerSsid, routerPassword)) {
    web.startWifiAttempt();
    WiFi.begin(routerSsid.c_str(), routerPassword.c_str());
    routerPassword = "";
  }

  web.begin();
  server.begin();
  Serial.println("Home Hub local site ready");
}

void loop() {
  radio.poll();
  HomeHubEnvironment::poll();
  server.handleClient();
  web.expireSession();
  yield();
}
