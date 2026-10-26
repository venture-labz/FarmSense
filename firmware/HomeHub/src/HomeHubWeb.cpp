#include "HomeHubWeb.h"
#include "HomeHubUi.h"
#include "HomeHubNetwork.h"
#include "HomeHubEnvironment.h"

#include <ctype.h>
#include <math.h>
#include <WiFi.h>

namespace {
bool validSensorId(const String& id) {
  if (id.length() == 0 || id.length() > 24) return false;
  for (size_t index = 0; index < id.length(); ++index) {
    const unsigned char value = id[index];
    if (!isalnum(value) && value != '-' && value != '_') return false;
  }
  return true;
}
}

namespace {
constexpr char kSessionCookie[] = "farmhub_session";

String jsonString(const char* value) {
  String encoded = "\"";
  for (const unsigned char* cursor = reinterpret_cast<const unsigned char*>(value);
       *cursor != 0; ++cursor) {
    if (*cursor == '"' || *cursor == '\\') {
      encoded += '\\';
      encoded += static_cast<char>(*cursor);
    } else if (*cursor < 0x20) {
      constexpr char hex[] = "0123456789abcdef";
      encoded += "\\u00";
      encoded += hex[*cursor >> 4];
      encoded += hex[*cursor & 0x0f];
    } else {
      encoded += static_cast<char>(*cursor);
    }
  }
  encoded += '"';
  return encoded;
}
}

HomeHubWeb::HomeHubWeb(WebServer& server, HomeHubStore& store, HomeHubRadio& radio)
    : server_(server), store_(store), radio_(radio) {}

void HomeHubWeb::begin() {
  const char* headers[] = {"Cookie"};
  server_.collectHeaders(headers, 1);
  server_.on("/", HTTP_GET, [this]() { serveRoot(); });
  server_.on("/api/auth/state", HTTP_GET, [this]() { serveAuthState(); });
  server_.on("/api/account", HTTP_POST, [this]() { createAccount(); });
  server_.on("/api/login", HTTP_POST, [this]() { login(); });
  server_.on("/api/logout", HTTP_POST, [this]() { logout(); });
  server_.on("/api/sensors", HTTP_GET, [this]() { serveSensors(); });
  server_.on("/api/events", HTTP_GET, [this]() { serveEvents(); });
  server_.on("/api/sensor/wait", HTTP_POST, [this]() { waitForSensor(); });
  server_.on("/api/sensor/action", HTTP_POST, [this]() { sensorAction(); });
  server_.on("/api/sensor/edit", HTTP_POST, [this]() { editSensor(); });
  server_.on("/api/network/wifi", HTTP_POST, [this]() { configureWifi(); });
  server_.on("/api/network/state", HTTP_GET, [this]() { serveNetworkState(); });
  server_.onNotFound([this]() {
    noStore();
    server_.send(404, "application/json", "{\"error\":\"Not found\"}");
  });
}

void HomeHubWeb::expireSession() { store_.expireSession(); }

void HomeHubWeb::startWifiAttempt() {
  wifiAttempted_ = true;
  wifiStartedMs_ = millis();
}

void HomeHubWeb::noStore() {
  server_.sendHeader("Cache-Control", "no-store, max-age=0");
}

String HomeHubWeb::requestCookie(const char* name) const {
  const String cookies = server_.header("Cookie");
  const String prefix = String(name) + "=";
  int start = cookies.indexOf(prefix);
  if (start < 0) return String();
  start += prefix.length();
  int end = cookies.indexOf(';', start);
  if (end < 0) end = cookies.length();
  return cookies.substring(start, end);
}

bool HomeHubWeb::requireSession() {
  if (store_.isSessionValid(requestCookie(kSessionCookie))) return true;
  noStore();
  server_.send(401, "application/json", "{\"error\":\"Sign in required\"}");
  return false;
}

void HomeHubWeb::serveRoot() {
  noStore();
  server_.send_P(200, "text/html; charset=utf-8", kPage);
}

void HomeHubWeb::serveAuthState() {
  const bool signedIn = store_.isSessionValid(requestCookie(kSessionCookie));
  noStore();
  server_.send(200, "application/json",
               String("{\"account\":") + (store_.hasAccount() ? "true" : "false") +
                   ",\"signed_in\":" + (signedIn ? "true" : "false") +
                   ",\"storage_ready\":" + (store_.isRegistryReady() ? "true" : "false") + "}");
}

void HomeHubWeb::serveNetworkState() {
  if (!requireSession()) return;
  const int status = WiFi.status();
  const bool connected = status == WL_CONNECTED;
  if (connected) wifiAttempted_ = false;
  const char* state = homeHubWifiState(status, wifiAttempted_, millis() - wifiStartedMs_);
  const bool connecting = strcmp(state, "connecting") == 0;
  const String ip = connected ? WiFi.localIP().toString() : String();
  noStore();
  server_.send(200, "application/json",
               String("{\"connected\":") + (connected ? "true" : "false") +
                   ",\"connecting\":" + (connecting ? "true" : "false") +
                   ",\"state\":" + jsonString(state) +
                   ",\"ip\":" + jsonString(ip.c_str()) + "}");
}

void HomeHubWeb::createAccount() {
  if (!store_.isReady()) {
    noStore();
    server_.send(503, "application/json", "{\"error\":\"Local storage unavailable\"}");
    return;
  }
  const String username = server_.arg("username");
  const String password = server_.arg("password");
  if (!store_.createAccount(username, password)) {
    noStore();
    server_.send(store_.hasAccount() ? 409 : 400, "application/json",
                 "{\"error\":\"Account setup failed; check values and local storage\"}");
    return;
  }
  const String token = store_.startSession();
  server_.sendHeader("Set-Cookie", String(kSessionCookie) + "=" + token +
                                       "; HttpOnly; SameSite=Strict; Path=/; Max-Age=3600");
  noStore();
  server_.send(201, "application/json", "{\"ok\":true}");
}

void HomeHubWeb::login() {
  if (!store_.verifyAccount(server_.arg("username"), server_.arg("password"))) {
    noStore();
    server_.send(401, "application/json", "{\"error\":\"Invalid username or password\"}");
    return;
  }
  const String token = store_.startSession();
  server_.sendHeader("Set-Cookie", String(kSessionCookie) + "=" + token +
                                       "; HttpOnly; SameSite=Strict; Path=/; Max-Age=3600");
  noStore();
  server_.send(200, "application/json", "{\"ok\":true}");
}

void HomeHubWeb::logout() {
  store_.endSession();
  server_.sendHeader("Set-Cookie", String(kSessionCookie) +
                                       "=; HttpOnly; SameSite=Strict; Path=/; Max-Age=0");
  noStore();
  server_.send(200, "application/json", "{\"ok\":true}");
}

void HomeHubWeb::serveSensors() {
  if (!requireSession()) return;
  if (!store_.isRegistryReady()) {
    noStore();
    server_.send(503, "application/json", "{\"error\":\"Sensor storage unavailable\"}");
    return;
  }
  String response = "{\"sensors\":[";
  for (size_t index = 0; index < store_.sensorCount(); ++index) {
    const SensorRecord* sensor = store_.sensorAt(index);
    if (index != 0) response += ',';
    response += "{\"id\":" + jsonString(sensor->id) + ",\"name\":" +
                jsonString(sensor->name) + ",\"type\":" + jsonString(sensor->type) +
                ",\"description\":" + jsonString(sensor->description) +
                ",\"primary_label\":" + jsonString(sensor->primaryLabel) +
                ",\"primary_value\":" + jsonString(sensor->primaryValue) +
                ",\"grouping\":" + jsonString(sensor->grouping) +
                ",\"location\":" + jsonString(sensor->location) +
                ",\"last_seen_ms\":" + String(sensor->lastSeenMs) +
                ",\"latitude_e6\":" + String(sensor->latitudeE6) +
                ",\"longitude_e6\":" + String(sensor->longitudeE6) +
                ",\"flags\":" + String(sensor->flags) + "}";
  }
  const auto& environment = HomeHubEnvironment::reading();
  response += "],\"home_hub\":{\"ready\":" + String(environment.ready ? "true" : "false") +
      ",\"temperature_c\":" + (environment.ready ? String(environment.temperatureC, 2) : String("null")) +
      ",\"pressure_hpa\":" + (environment.ready ? String(environment.pressureHpa, 2) : String("null")) +
      ",\"last_seen_ms\":" + String(environment.lastSeenMs) + "},\"uptime_ms\":" + String(millis()) + "}";
  noStore();
  server_.send(200, "application/json", response);
}

void HomeHubWeb::serveEvents() {
  if (!requireSession()) return;
  String response = "{\"events\":[";
  for (size_t i = 0; i < store_.eventCount(); ++i) {
    const HubEvent& event = store_.eventAt(i);
    if (i) response += ',';
    response += "{\"sequence\":" + String(event.sequence) +
        ",\"received_ms\":" + String(event.receivedMs) +
        ",\"sensor_id\":" + jsonString(event.sensorId) +
        ",\"message\":" + jsonString(event.message) + "}";
  }
  response += "],\"uptime_ms\":" + String(millis()) + "}";
  noStore();
  server_.send(200, "application/json", response);
}

void HomeHubWeb::waitForSensor() {
  if (!requireSession()) return;
  const String id = server_.arg("id");
  if (!validSensorId(id)) {
    noStore();
    server_.send(400, "application/json", "{\"error\":\"Invalid sensor ID\"}");
    return;
  }
  SensorRecord* existing = store_.findSensor(id.c_str());
  if (existing != nullptr && (existing->flags & kSensorClaimed) != 0) {
    noStore();
    server_.send(409, "application/json", "{\"error\":\"Sensor is already claimed\"}");
    return;
  }
  SensorRecord sensor{};
  if (existing != nullptr) sensor = *existing;
  else {
    strlcpy(sensor.id, id.c_str(), sizeof(sensor.id));
    strlcpy(sensor.name, id.c_str(), sizeof(sensor.name));
    sensor.radioAddress = UINT16_MAX;
  }
  addOwnedSensor(sensor);
  if (!store_.upsertSensor(sensor, true)) {
    noStore();
    server_.send(507, "application/json", "{\"error\":\"Sensor registry full or unavailable\"}");
    return;
  }
  if (sensor.flags & kSensorDescriptorReceived) radio_.sendSetupStatus(sensor);
  noStore();
  server_.send(202, "application/json", "{\"ok\":true,\"status\":\"added\"}");
}

void HomeHubWeb::sensorAction() {
  if (!requireSession()) return;
  const String id = server_.arg("id");
  const String action = server_.arg("action");
  if (!validSensorId(id)) {
    noStore();
    server_.send(400, "application/json", "{\"error\":\"Invalid sensor ID\"}");
    return;
  }
  SensorRecord* sensor = store_.findSensor(id.c_str());
  if (sensor == nullptr) {
    noStore();
    server_.send(404, "application/json", "{\"error\":\"Sensor not found\"}");
    return;
  }
  if (action == "claim") {
    addOwnedSensor(*sensor);
  } else if (action == "ignore") {
    sensor->flags |= kSensorIgnored;
  } else if (action == "restore") {
    sensor->flags &= static_cast<uint8_t>(~kSensorIgnored);
  } else {
    noStore();
    server_.send(400, "application/json", "{\"error\":\"Unknown sensor action\"}");
    return;
  }
  if (!store_.saveRegistry()) {
    noStore();
    server_.send(507, "application/json", "{\"error\":\"Could not save sensor state\"}");
    return;
  }
  if (action == "claim") radio_.sendSetupStatus(*sensor);
  noStore();
  server_.send(200, "application/json", "{\"ok\":true}");
}

void HomeHubWeb::editSensor() {
  if (!requireSession()) return;
  const String id = server_.arg("id");
  SensorRecord* sensor = store_.findSensor(id.c_str());
  if (!validSensorId(id) || sensor == nullptr || (sensor->flags & kSensorClaimed) == 0) {
    noStore();
    server_.send(sensor == nullptr ? 404 : 400, "application/json",
                 "{\"error\":\"A claimed sensor is required\"}");
    return;
  }
  const String name = server_.arg("name");
  const String grouping = server_.arg("grouping");
  const String location = server_.arg("location");
  const String latitude = server_.arg("latitude");
  const String longitude = server_.arg("longitude");
  if (name.length() == 0 || name.length() >= sizeof(sensor->name) ||
      grouping.length() >= sizeof(sensor->grouping) || location.length() >= sizeof(sensor->location) ||
      latitude.length() > 16 || longitude.length() > 16 ||
      (latitude.length() == 0) != (longitude.length() == 0)) {
    noStore();
    server_.send(400, "application/json", "{\"error\":\"Invalid sensor properties\"}");
    return;
  }
  int32_t latitudeE6 = 0;
  int32_t longitudeE6 = 0;
  if (latitude.length() > 0) {
    char* end = nullptr;
    const float lat = strtof(latitude.c_str(), &end);
    if (*end != '\0' || !isfinite(lat) || lat < -90.0f || lat > 90.0f) {
      noStore(); server_.send(400, "application/json", "{\"error\":\"Invalid coordinates\"}"); return;
    }
    const float lon = strtof(longitude.c_str(), &end);
    if (*end != '\0' || !isfinite(lon) || lon < -180.0f || lon > 180.0f) {
      noStore(); server_.send(400, "application/json", "{\"error\":\"Invalid coordinates\"}"); return;
    }
    latitudeE6 = static_cast<int32_t>(lat * 1000000.0f);
    longitudeE6 = static_cast<int32_t>(lon * 1000000.0f);
  }
  strlcpy(sensor->name, name.c_str(), sizeof(sensor->name));
  strlcpy(sensor->grouping, grouping.c_str(), sizeof(sensor->grouping));
  strlcpy(sensor->location, location.c_str(), sizeof(sensor->location));
  sensor->latitudeE6 = latitudeE6;
  sensor->longitudeE6 = longitudeE6;
  if (latitude.length() > 0) sensor->flags |= kSensorHasLocation;
  else sensor->flags &= static_cast<uint8_t>(~kSensorHasLocation);
  const bool alarmEnabled = server_.arg("alarm_enabled") == "on";
  if (alarmEnabled) sensor->flags |= kSensorAlarmEnabled;
  else sensor->flags &= static_cast<uint8_t>(~kSensorAlarmEnabled);
  if (!store_.saveRegistry()) {
    noStore(); server_.send(507, "application/json", "{\"error\":\"Could not save sensor properties\"}"); return;
  }
  noStore();
  server_.send(200, "application/json", "{\"ok\":true}");
}

void HomeHubWeb::configureWifi() {
  if (!requireSession()) return;
  const String ssid = server_.arg("ssid");
  const String password = server_.arg("password");
  if (ssid.length() == 0 || ssid.length() > 32 || password.length() > 64) {
    noStore(); server_.send(400, "application/json", "{\"error\":\"Invalid Wi-Fi settings\"}"); return;
  }
  if (!store_.saveRouterCredentials(ssid, password)) {
    noStore(); server_.send(507, "application/json", "{\"error\":\"Could not save Wi-Fi settings\"}"); return;
  }
  WiFi.mode(WIFI_AP_STA);
  WiFi.disconnect(false, false);
  startWifiAttempt();
  WiFi.begin(ssid.c_str(), password.c_str());
  noStore();
  server_.send(202, "application/json", "{\"ok\":true,\"status\":\"connecting\"}");
}
