#include <Arduino.h>
#include <Preferences.h>

// Service-only image. Restore normal HomeHub firmware after running.
// No radio, network, registry reads, or credential reads.
void setup() {
  Serial.begin(115200);
  Preferences preferences;
  if (!preferences.begin("homehub", false)) {
    Serial.println("Account reset failed: storage unavailable");
    return;
  }
  bool removed = true;
  for (const char* key : {"username", "salt", "passhash"}) {
    if (preferences.isKey(key) && !preferences.remove(key)) removed = false;
  }
  const bool reset = removed && preferences.putBool("account", false);
  preferences.end();
  Serial.println(reset ? "Local account reset complete" : "Local account reset failed");
}

void loop() { delay(100); }
