// Farm Sense peripheral integration validation.
// Target: Seeed Studio XIAO ESP32-C3.
// D1 indicator; D4 SCL; D5 SDA; D6 RYLR998 RXD; D7 RYLR998 TXD.
// This is diagnostic firmware. It does not write MCP23017 GPIO registers or
// modify persistent RYLR998 settings.

#include <Arduino.h>
#include <Preferences.h>
#include <WebServer.h>
#include <WiFi.h>
#include <Wire.h>

namespace {
constexpr uint8_t kIndicatorPin = D1;
constexpr uint8_t kI2cSclPin = D4;
constexpr uint8_t kI2cSdaPin = D5;
constexpr uint8_t kLoraTxPin = D6;  // Connected to the RYLR998 RXD input.
constexpr uint8_t kLoraRxPin = D7;  // Connected to the RYLR998 TXD output.

constexpr uint8_t kMcp23017Address = 0x20;
constexpr uint8_t kBmp280Address = 0x76;
constexpr uint8_t kBmp280ChipId = 0x58;
constexpr uint8_t kBme280ChipId = 0x60;
constexpr uint32_t kLoraBaudRate = 115'200;
constexpr uint32_t kSensorIntervalMs = 2'000;
constexpr uint32_t kMcpIntervalMs = 10'000;
constexpr uint32_t kLoraHealthRetryMs = 15'000;
constexpr uint32_t kActivityFlashMs = 120;
constexpr uint32_t kLedFrequencyHz = 5'000;
constexpr uint8_t kLedResolutionBits = 8;
constexpr uint8_t kLedIdleDuty = 127;    // 50% lit.
constexpr uint8_t kLedActivityDuty = 204;  // 80% lit.
constexpr char kAccessPointSsid[] = "FarmSense-HomeHub";
constexpr char kGateSensorId[] = "gate-01";
constexpr uint32_t kDefaultGateAliveSeconds = 3'600;
constexpr uint32_t kMinimumGateAliveSeconds = 5;
constexpr uint32_t kMaximumGateAliveSeconds = 86'400;

WebServer statusServer(80);
Preferences hubPreferences;

struct BmpCalibration {
  uint16_t t1;
  int16_t t2;
  int16_t t3;
  uint16_t p1;
  int16_t p2;
  int16_t p3;
  int16_t p4;
  int16_t p5;
  int16_t p6;
  int16_t p7;
  int16_t p8;
  int16_t p9;
};

BmpCalibration bmpCalibration{};
bool bmpReady = false;
bool loraHealthy = false;
bool mcpDetected = false;
bool hasBmpReading = false;
float lastTemperatureC = 0.0F;
float lastPressureHpa = 0.0F;
String lastLoraReceive = "No LoRa packet received since boot";
int gateRaw = -1;
// DRV5032FALPGM output is LOW while the magnet is detected.
constexpr int kGateClosedRaw = LOW;
int gateClosedRaw = kGateClosedRaw;
bool gateAlarmEnabled = true;
uint32_t gateAliveSeconds = kDefaultGateAliveSeconds;
uint32_t gateConfigRevision = 0;
int gateRadioAddress = -1;
uint32_t gateLastPacketMs = 0;
uint32_t gateLastSensorSentMs = 0;
int64_t gateSensorToHubOffsetMs = 0;
uint32_t gateEstimatedTransitMs = 0;
uint32_t gateLastSyncMs = 0;
String gateLastKind = "No Gate Sensor packet received";
String gateConfigStatus = "Awaiting Gate Sensor report";
String gateHistory[8];
uint8_t gateHistoryCount = 0;
String incomingLog[16];
uint8_t incomingLogCount = 0;
uint32_t activityUntilMs = 0;
uint32_t lastSensorMs = 0;
uint32_t lastMcpMs = 0;
uint32_t lastLoraHealthMs = 0;
String loraLine;

uint16_t readU16LE(const uint8_t* bytes) {
  return static_cast<uint16_t>(bytes[0]) |
         (static_cast<uint16_t>(bytes[1]) << 8);
}

String jsonEscape(const String& value) {
  String escaped;
  escaped.reserve(value.length() + 8);
  for (size_t index = 0; index < value.length(); ++index) {
    const char character = value[index];
    switch (character) {
      case '\\': escaped += "\\\\"; break;
      case '"': escaped += "\\\""; break;
      case '\n': escaped += "\\n"; break;
      case '\r': escaped += "\\r"; break;
      case '\t': escaped += "\\t"; break;
      default:
        if (static_cast<uint8_t>(character) >= 0x20) {
          escaped += character;
        }
    }
  }
  return escaped;
}

void flashActivity();

String hubTimestamp() {
  const uint32_t totalSeconds = millis() / 1000;
  char formatted[12];
  snprintf(formatted, sizeof(formatted), "%02lu:%02lu:%02lu",
           static_cast<unsigned long>(totalSeconds / 3600),
           static_cast<unsigned long>((totalSeconds / 60) % 60),
           static_cast<unsigned long>(totalSeconds % 60));
  return String(formatted);
}

void rememberIncomingLog(const String& sensor, const String& data) {
  for (uint8_t index = 15; index > 0; --index) {
    incomingLog[index] = incomingLog[index - 1];
  }
  incomingLog[0] = hubTimestamp() + " | " + sensor + " | " + data;
  if (incomingLogCount < 16) {
    ++incomingLogCount;
  }
}

String gateState() {
  if (gateRaw < 0) {
    return "UNCONFIGURED";
  }
  return gateRaw == gateClosedRaw ? "CLOSED" : "OPEN";
}

bool gateAlarmActive() {
  return gateAlarmEnabled && gateRaw >= 0 &&
         gateRaw != gateClosedRaw;
}

void rememberGateEvent(const String& event) {
  for (uint8_t index = 7; index > 0; --index) {
    gateHistory[index] = gateHistory[index - 1];
  }
  gateHistory[0] = event;
  if (gateHistoryCount < 8) {
    ++gateHistoryCount;
  }
}

void saveGateSettings() {
  hubPreferences.putBool("alarm", gateAlarmEnabled);
  hubPreferences.putUInt("alive_sec", gateAliveSeconds);
  hubPreferences.putUInt("cfg_rev", gateConfigRevision);
}

bool sendGateConfiguration() {
  if (gateRadioAddress < 0) {
    gateConfigStatus = "Queued: waiting for Gate Sensor report";
    return false;
  }
  const String payload = String("FS2|CFG|") + kGateSensorId + "|" +
                         String(gateConfigRevision) + "|" + String(gateClosedRaw) +
                         "|" + (gateAlarmEnabled ? "1" : "0") + "|" +
                         String(gateAliveSeconds);
  Serial1.print("AT+SEND=");
  Serial1.print(gateRadioAddress);
  Serial1.print(',');
  Serial1.print(payload.length());
  Serial1.print(',');
  Serial1.print(payload);
  Serial1.print("\r\n");
  gateConfigStatus = "Configuration sent; awaiting acknowledgement";
  Serial.print("Gate configuration: ");
  Serial.println(payload);
  flashActivity();
  return true;
}

void sendGateEventReceipt(const int destinationAddress, const String& eventSequence) {
  const String payload = String("FS2|RCPT|") + kGateSensorId + "|" + eventSequence + "|OK";
  Serial1.print("AT+SEND=");
  Serial1.print(destinationAddress);
  Serial1.print(',');
  Serial1.print(payload.length());
  Serial1.print(',');
  Serial1.print(payload);
  Serial1.print("\r\n");
  flashActivity();
}

void requestGateClockSync() {
  if (gateRadioAddress < 0 || millis() - gateLastSyncMs < 5'000) return;
  gateLastSyncMs = millis();
  const String payload = String("FS2|SYNC|") + kGateSensorId + "|" + String(gateLastSyncMs);
  Serial1.print("AT+SEND="); Serial1.print(gateRadioAddress); Serial1.print(',');
  Serial1.print(payload.length()); Serial1.print(','); Serial1.print(payload); Serial1.print("\r\n");
}

void serveStatusPage() {
  Serial.println("HTTP GET /");
  constexpr char kPage[] = R"HTML(<!doctype html><html lang="en"><head><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1"><title>Farm Sense Home Hub</title><style>body{margin:0;background:#101714;color:#e6f2eb;font:16px system-ui,sans-serif}main{max-width:760px;margin:auto;padding:24px}h1{margin:0 0 4px}.sub,.note{color:#adc5b5}.grid{display:grid;grid-template-columns:repeat(auto-fit,minmax(210px,1fr));gap:12px;margin-top:22px}.card{background:#19231d;border:1px solid #385341;border-radius:8px;padding:15px}.label{color:#a9c1b0;font-size:.85rem}.value{font:600 1.15rem ui-monospace,monospace;margin-top:6px}.packet{white-space:pre-wrap;word-break:break-word}.alert{border-color:#d68437;background:#35220f}.hidden{display:none}button,input{font:inherit;padding:8px}button{background:#4e8c5f;color:#fff;border:0;border-radius:5px}input{width:7em}.row{display:flex;gap:10px;align-items:center;flex-wrap:wrap;margin-top:12px}</style></head><body><main><h1>Farm Sense Home Hub</h1><div class="sub" id="connection">Loading local status…</div><section class="grid"><div class="card"><div class="label">Temperature</div><div class="value" id="temperature">—</div></div><div class="card"><div class="label">Pressure</div><div class="value" id="pressure">—</div></div><div class="card"><div class="label">MCP23017</div><div class="value" id="mcp">—</div></div><div class="card"><div class="label">RYLR998</div><div class="value" id="lora">—</div></div></section><section class="card" id="gate" style="margin-top:12px"><div class="label">Gate Sensor 01</div><div class="value" id="gateState">Awaiting report</div><div class="sub" id="gateRaw">Raw output: —</div><div class="sub" id="gateMeta">—</div><div class="row"><button id="calibrate">Set CLOSED now</button><label><input id="alarm" type="checkbox" checked> alarm when OPEN</label><label>ALIVE min <input id="alive" type="number" min="1" max="1440" value="60"></label><button id="save">Save settings</button></div><div class="sub" id="config">—</div><div class="label" style="margin-top:12px">Raw transition history</div><div class="packet sub" id="history">—</div></section><section class="card" style="margin-top:12px"><div class="label">Most recent LoRa event</div><div class="value packet" id="packet">—</div></section><p class="note">Local testing interface · settings are unauthenticated while the SoftAP is open</p></main><script>async function request(url,opt){const r=await fetch(url,opt);if(!r.ok)throw Error('request failed');return r.json()}async function refresh(){try{const s=await request('/api/status',{cache:'no-store'});temperature.textContent=s.bmp.ready?`${s.bmp.temperature_c.toFixed(2)} °C`:'Unavailable';pressure.textContent=s.bmp.ready?`${s.bmp.pressure_hpa.toFixed(2)} hPa`:'Unavailable';mcp.textContent=s.mcp_detected?'Detected':'Not detected';lora.textContent=s.lora_healthy?'UART healthy':'No response';packet.textContent=s.last_lora;connection.textContent=`SoftAP active · uptime ${Math.floor(s.uptime_ms/1000)} s`;const g=s.gate;gateState.textContent=g.state;gateRaw.textContent=g.raw<0?'Raw output: awaiting report':`Raw output: ${g.raw?'HIGH':'LOW'} (${g.raw})`;gateMeta.textContent=`Last ${g.last_kind} · ${Math.floor(g.last_packet_ms/1000)} s since boot`;alarm.checked=g.alarm_enabled;alive.value=g.alive_minutes;config.textContent=g.config_status;history.textContent=g.history.length?g.history.join('\n'):'No transitions yet';gate.classList.toggle('alert',g.alarm_active)}catch(e){connection.textContent='Status unavailable; retrying…'}}calibrate.onclick=async()=>{await request('/api/gate/calibrate-closed',{method:'POST'});refresh()};save.onclick=async()=>{const body=new URLSearchParams({alarm:alarm.checked?'1':'0',alive_minutes:alive.value});await request('/api/gate/settings',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body});refresh()};refresh();setInterval(refresh,2000)</script></body></html>)HTML";
  statusServer.send(200, "text/html; charset=utf-8", kPage);
}

void serveEnhancedStatusPage() {
  Serial.println("HTTP GET /");
  constexpr char kPage[] = R"HTML(<!doctype html><html lang="en"><head><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1"><title>Farm Sense Home Hub</title><style>body{margin:0;background:#101714;color:#e6f2eb;font:16px system-ui,sans-serif}main{max-width:800px;margin:auto;padding:24px}h1{margin:0 0 4px}.sub,.note{color:#adc5b5}.grid{display:grid;grid-template-columns:repeat(auto-fit,minmax(210px,1fr));gap:12px;margin-top:22px}.card{background:#19231d;border:1px solid #385341;border-radius:8px;padding:15px;margin-top:12px}.label{color:#a9c1b0;font-size:.85rem}.value{font:600 1.15rem ui-monospace,monospace;margin-top:6px}.packet{white-space:pre-wrap;word-break:break-word}.alert{border-color:#d68437;background:#35220f}button,input{font:inherit;padding:8px}button{background:#4e8c5f;color:#fff;border:0;border-radius:5px}.row{display:flex;gap:10px;align-items:center;flex-wrap:wrap;margin-top:12px}.log{max-height:290px;overflow:auto;background:#0c120f;padding:10px;border-radius:4px;font:.82rem ui-monospace,monospace;white-space:pre-wrap}</style></head><body><main><h1>Farm Sense Home Hub</h1><div class="sub" id="connection">Loading local status…</div><section class="grid"><div class="card"><div class="label">Temperature</div><div class="value" id="temperature">—</div></div><div class="card"><div class="label">Pressure</div><div class="value" id="pressure">—</div></div><div class="card"><div class="label">MCP23017</div><div class="value" id="mcp">—</div></div><div class="card"><div class="label">RYLR998</div><div class="value" id="lora">—</div></div></section><section class="card" id="gate"><div class="label">Gate Sensor 01</div><div class="value" id="gateState">Awaiting report</div><div class="sub" id="gateRaw">Raw output: —</div><div class="sub" id="gateMeta">—</div><div class="row"><button id="calibrate">Set CLOSED now</button><label><input id="alarm" type="checkbox" checked> alarm when OPEN</label><label>ALIVE seconds <input id="alive" type="number" min="5" max="86400" value="3600"></label><button id="save">Save settings</button></div><div class="sub" id="config">—</div><div class="label" style="margin-top:12px">Gate transition history</div><div class="packet sub" id="history">—</div></section><section class="card"><div class="label">Most recent LoRa event</div><div class="value packet" id="packet">—</div></section><section class="card"><div class="label">Incoming data log</div><div class="sub">Timestamp since Hub boot | Sensor | Data</div><div class="log" id="incoming">Awaiting incoming data…</div></section><p class="note">Local testing interface · settings are unauthenticated while the SoftAP is open</p></main><script>async function request(url,opt){const r=await fetch(url,opt);if(!r.ok)throw Error('request failed');return r.json()}async function refresh(){try{const s=await request('/api/status',{cache:'no-store'});temperature.textContent=s.bmp.ready?`${s.bmp.temperature_c.toFixed(2)} °C`:'Unavailable';pressure.textContent=s.bmp.ready?`${s.bmp.pressure_hpa.toFixed(2)} hPa`:'Unavailable';mcp.textContent=s.mcp_detected?'Detected':'Not detected';lora.textContent=s.lora_healthy?'UART healthy':'No response';packet.textContent=s.last_lora;connection.textContent=`SoftAP active · uptime ${Math.floor(s.uptime_ms/1000)} s`;const g=s.gate;gateState.textContent=g.state;gateRaw.textContent=g.raw<0?'Raw output: awaiting report':`Raw output: ${g.raw?'HIGH':'LOW'} (${g.raw})`;gateMeta.textContent=`Last ${g.last_kind} · ${Math.floor(g.last_packet_ms/1000)} s since boot`;alarm.checked=g.alarm_enabled;alive.value=g.alive_seconds;config.textContent=g.config_status;history.textContent=g.history.length?g.history.join('\n'):'No transitions yet';incoming.textContent=s.incoming_log.length?s.incoming_log.join('\n'):'Awaiting incoming data…';gate.classList.toggle('alert',g.alarm_active)}catch(e){connection.textContent='Status unavailable; retrying…'}}calibrate.onclick=async()=>{await request('/api/gate/calibrate-closed',{method:'POST'});refresh()};save.onclick=async()=>{const body=new URLSearchParams({alarm:alarm.checked?'1':'0',alive_seconds:alive.value});await request('/api/gate/settings',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body});refresh()};refresh();setInterval(refresh,2000)</script></body></html>)HTML";
  statusServer.send(200, "text/html; charset=utf-8", kPage);
}

void serveFastStatusPage() {
  Serial.println("HTTP GET /");
  constexpr char kPage[] = R"HTML(<!doctype html><meta name="viewport" content="width=device-width,initial-scale=1"><title>Farm Sense</title><style>body{margin:0;background:#101714;color:#e6f2eb;font:16px system-ui}main{max-width:800px;margin:auto;padding:20px}.card{background:#19231d;border:1px solid #385341;border-radius:8px;padding:14px;margin:12px 0}.value{font:600 1.2rem ui-monospace,monospace}.sub,pre{color:#adc5b5}button,input{font:inherit;padding:7px}button{background:#4e8c5f;color:#fff;border:0;border-radius:5px}.alert{border-color:#d68437;background:#35220f}pre{max-height:280px;overflow:auto;white-space:pre-wrap}</style><main><h1>Farm Sense Home Hub</h1><div class="card" id="gate"><b>Gate Sensor 01</b><div class="value" id="state">Awaiting report</div><div class="sub" id="raw">Raw output: —</div><p><button id="closed">Set CLOSED now</button> <label><input id="alarm" type="checkbox" checked> alarm when OPEN</label> <label>ALIVE seconds <input id="alive" type="number" min="5" max="86400" value="3600"></label> <button id="save">Save</button></p><div class="sub" id="config"></div></div><div class="card"><b>Incoming data log</b><pre id="log">Awaiting data…</pre></div></main><script>async function req(u,o){let r=await fetch(u,o);if(!r.ok)throw Error();return r.json()}async function refresh(){try{let s=await req('/api/status',{cache:'no-store'}),g=s.gate;state.textContent=g.state;raw.textContent=g.raw<0?'Raw output: awaiting report':`Raw output: ${g.raw?'HIGH':'LOW'} (${g.raw})`;config.textContent=g.config_status;alarm.checked=g.alarm_enabled;alive.value=g.alive_seconds;log.textContent=s.incoming_log.length?s.incoming_log.join('\n'):'Awaiting data…';gate.classList.toggle('alert',g.alarm_active)}catch(e){config.textContent='Connection retrying…'}}closed.onclick=async()=>{await req('/api/gate/calibrate-closed',{method:'POST'});refresh()};save.onclick=async()=>{let b=new URLSearchParams({alarm:alarm.checked?'1':'0',alive_seconds:alive.value});await req('/api/gate/settings',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:b});refresh()};refresh();setInterval(refresh,500)</script>)HTML";
  statusServer.send(200, "text/html; charset=utf-8", kPage);
}

void serveEditableStatusPage() {
  constexpr char kPage[] = R"HTML(<!doctype html><meta name="viewport" content="width=device-width,initial-scale=1"><title>Farm Sense</title><style>body{margin:0;background:#101714;color:#e6f2eb;font:16px system-ui}main{max-width:800px;margin:auto;padding:20px}.card{background:#19231d;border:1px solid #385341;border-radius:8px;padding:14px;margin:12px 0}.value{font:600 1.2rem ui-monospace,monospace}.sub,pre{color:#adc5b5}button,input{font:inherit;padding:7px}button{background:#4e8c5f;color:#fff;border:0;border-radius:5px}.alert{border-color:#d68437;background:#35220f}pre{max-height:280px;overflow:auto;white-space:pre-wrap}.ok{color:#8fd69e}.err{color:#ffad81}</style><main><h1>Farm Sense Home Hub</h1><div class="card" id="gate"><b>Gate Sensor 01</b><div class="value" id="state">Awaiting report</div><div class="sub" id="raw">Raw output: —</div><p><button id="closed" type="button">Set CLOSED now</button></p><form id="settings"><label><input id="alarm" type="checkbox" checked> alarm when OPEN</label> <label>ALIVE seconds <input id="alive" type="number" min="5" max="86400" required value="3600"></label> <button id="save" type="submit">Save settings</button></form><div class="sub" id="saveNote" aria-live="polite">Change a value, then save it.</div><div class="sub" id="config"></div></div><div class="card"><b>Incoming data log</b><pre id="log">Awaiting data…</pre></div></main><script>let dirty=false;const el=id=>document.getElementById(id),gate=el('gate'),state=el('state'),raw=el('raw'),alarm=el('alarm'),alive=el('alive'),config=el('config'),log=el('log'),save=el('save'),saveNote=el('saveNote'),settings=el('settings'),closed=el('closed');async function req(u,o){let r=await fetch(u,o);let j=await r.json();if(!r.ok)throw Error(j.error||'Request failed');return j}async function refresh(){try{let s=await req('/api/status',{cache:'no-store'}),g=s.gate;state.textContent=g.state;raw.textContent=g.raw<0?'Raw output: awaiting report':`Raw output: ${g.raw?'HIGH':'LOW'} (${g.raw})`;config.textContent=g.config_status;log.textContent=s.incoming_log.length?s.incoming_log.join('\n'):'Awaiting data…';gate.classList.toggle('alert',g.alarm_active);if(!dirty){alarm.checked=g.alarm_enabled;alive.value=g.alive_seconds}}catch(e){config.textContent='Connection retrying…'}}alive.addEventListener('input',()=>dirty=true);alarm.addEventListener('change',()=>dirty=true);closed.addEventListener('click',async()=>{try{await req('/api/gate/calibrate-closed',{method:'POST'});saveNote.className='sub ok';saveNote.textContent='CLOSED reference saved and sent to Gate Sensor 01.';refresh()}catch(e){saveNote.className='sub err';saveNote.textContent=e.message}});settings.addEventListener('submit',async e=>{e.preventDefault();let seconds=Number(alive.value);if(!Number.isInteger(seconds)||seconds<5||seconds>86400){saveNote.className='sub err';saveNote.textContent='Enter whole seconds from 5 to 86400.';return}save.disabled=true;saveNote.className='sub';saveNote.textContent='Saving…';try{let s=await req('/api/gate/settings',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:new URLSearchParams({alarm:alarm.checked?'1':'0',alive_seconds:String(seconds)})}),g=s.gate;dirty=false;alarm.checked=g.alarm_enabled;alive.value=g.alive_seconds;saveNote.className='sub ok';saveNote.textContent=`Saved ${g.alive_seconds} seconds to the Home Hub. ${g.config_status}`;refresh()}catch(e){saveNote.className='sub err';saveNote.textContent=`Not saved: ${e.message}`;dirty=true}finally{save.disabled=false}});refresh();setInterval(refresh,500)</script>)HTML";
  statusServer.sendHeader("Cache-Control", "no-store, max-age=0");
  statusServer.send(200, "text/html; charset=utf-8", kPage);
}

void serveFixedGateStatusPage() {
  constexpr char kPage[] = R"HTML(<!doctype html><meta name="viewport" content="width=device-width,initial-scale=1"><title>Farm Sense</title><style>body{margin:0;background:#101714;color:#e6f2eb;font:16px system-ui}main{max-width:800px;margin:auto;padding:20px}.card{background:#19231d;border:1px solid #385341;border-radius:8px;padding:14px;margin:12px 0}.value{font:600 1.2rem ui-monospace,monospace}.sub,pre{color:#adc5b5}button,input{font:inherit;padding:7px}button{background:#4e8c5f;color:#fff;border:0;border-radius:5px}.alert{border-color:#d68437;background:#35220f}pre{max-height:280px;overflow:auto;white-space:pre-wrap}.ok{color:#8fd69e}.err{color:#ffad81}</style><main><h1>Farm Sense Home Hub</h1><div class="card" id="gate"><b>Gate Sensor 01</b><div class="value" id="state">Awaiting report</div><div class="sub" id="raw">Raw output: —</div><div class="sub">Fixed mapping: magnet detected (LOW / 0) = CLOSED</div><form id="settings"><p><label><input id="alarm" type="checkbox" checked> alarm when OPEN</label> <label>ALIVE seconds <input id="alive" type="number" min="5" max="86400" required value="3600"></label> <button id="save" type="submit">Save settings</button></p></form><div class="sub" id="saveNote" aria-live="polite">Change a value, then save it.</div><div class="sub" id="config"></div></div><div class="card"><b>Incoming data log</b><pre id="log">Awaiting data…</pre></div></main><script>let dirty=false;const el=id=>document.getElementById(id),gate=el('gate'),state=el('state'),raw=el('raw'),alarm=el('alarm'),alive=el('alive'),config=el('config'),log=el('log'),save=el('save'),saveNote=el('saveNote'),settings=el('settings');async function req(u,o){let r=await fetch(u,o),j=await r.json();if(!r.ok)throw Error(j.error||'Request failed');return j}async function refresh(){try{let s=await req('/api/status',{cache:'no-store'}),g=s.gate;state.textContent=g.state;raw.textContent=g.raw<0?'Raw output: awaiting report':`Raw output: ${g.raw?'HIGH':'LOW'} (${g.raw})`;config.textContent=g.config_status;log.textContent=s.incoming_log.length?s.incoming_log.join('\n'):'Awaiting data…';gate.classList.toggle('alert',g.alarm_active);if(!dirty){alarm.checked=g.alarm_enabled;alive.value=g.alive_seconds}}catch(e){config.textContent='Connection retrying…'}}alive.addEventListener('input',()=>dirty=true);alarm.addEventListener('change',()=>dirty=true);settings.addEventListener('submit',async e=>{e.preventDefault();let seconds=Number(alive.value);if(!Number.isInteger(seconds)||seconds<5||seconds>86400){saveNote.className='sub err';saveNote.textContent='Enter whole seconds from 5 to 86400.';return}save.disabled=true;saveNote.className='sub';saveNote.textContent='Saving…';try{let s=await req('/api/gate/settings',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:new URLSearchParams({alarm:alarm.checked?'1':'0',alive_seconds:String(seconds)})}),g=s.gate;dirty=false;alarm.checked=g.alarm_enabled;alive.value=g.alive_seconds;saveNote.className='sub ok';saveNote.textContent=`Saved ${g.alive_seconds} seconds to the Home Hub. ${g.config_status}`;refresh()}catch(e){saveNote.className='sub err';saveNote.textContent=`Not saved: ${e.message}`;dirty=true}finally{save.disabled=false}});refresh();setInterval(refresh,500)</script>)HTML";
  statusServer.sendHeader("Cache-Control", "no-store, max-age=0");
  statusServer.send(200, "text/html; charset=utf-8", kPage);
}

void serveResponsiveGateStatusPage() {
  constexpr char kPage[] = R"HTML(<!doctype html><meta name="viewport" content="width=device-width,initial-scale=1"><title>Farm Sense</title><style>body{margin:0;background:#101714;color:#e6f2eb;font:16px system-ui}main{max-width:800px;margin:auto;padding:20px}.card{background:#19231d;border:1px solid #385341;border-radius:8px;padding:14px;margin:12px 0}.value{font:600 1.2rem ui-monospace,monospace}.sub,pre{color:#adc5b5}button,input{font:inherit;padding:7px}button{background:#4e8c5f;color:#fff;border:0;border-radius:5px}.alert{border-color:#d68437;background:#35220f}pre{max-height:280px;overflow:auto;white-space:pre-wrap}.ok{color:#8fd69e}.err{color:#ffad81}</style><main><h1>Farm Sense Home Hub</h1><div class="card" id="gate"><b>Gate Sensor 01</b><div class="value" id="state">Connecting…</div><div class="sub" id="raw">Raw output: —</div><div class="sub">Fixed mapping: magnet detected (LOW / 0) = CLOSED</div><form id="settings"><p><label><input id="alarm" type="checkbox"> alarm when OPEN</label> <label>ALIVE seconds <input id="alive" type="number" min="5" max="86400" required></label> <button id="save" type="submit">Save settings</button></p></form><div class="sub" id="note"></div><div class="sub" id="config"></div></div><div class="card"><b>Incoming data log</b><pre id="log">Awaiting data…</pre></div></main><script>let dirty=false;const $=id=>document.getElementById(id),gate=$('gate'),state=$('state'),raw=$('raw'),alarm=$('alarm'),alive=$('alive'),config=$('config'),log=$('log'),note=$('note'),save=$('save'),settings=$('settings');async function req(url,opt){let ctrl=new AbortController,t=setTimeout(()=>ctrl.abort(),1200);try{let r=await fetch(url,{...opt,cache:'no-store',signal:ctrl.signal}),j=await r.json();if(!r.ok)throw Error(j.error||'Request failed');return j}finally{clearTimeout(t)}}function paint(s){let g=s.gate;state.textContent=g.state;raw.textContent=g.raw<0?'Raw output: awaiting report':`Raw output: ${g.raw?'HIGH':'LOW'} (${g.raw})`;config.textContent=g.config_status;log.textContent=s.incoming_log.length?s.incoming_log.join('\n'):'Awaiting data…';gate.classList.toggle('alert',g.alarm_active);if(!dirty){alarm.checked=g.alarm_enabled;alive.value=g.alive_seconds}}async function refresh(){try{paint(await req('/api/status'))}catch(e){config.textContent='Connection retrying…'}finally{setTimeout(refresh,250)}}alive.oninput=()=>dirty=true;alarm.onchange=()=>dirty=true;settings.onsubmit=async e=>{e.preventDefault();let seconds=Number(alive.value);if(!Number.isInteger(seconds)||seconds<5||seconds>86400){note.className='sub err';note.textContent='Enter whole seconds from 5 to 86400.';return}save.disabled=true;note.className='sub';note.textContent='Saving…';try{let s=await req('/api/gate/settings',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:new URLSearchParams({alarm:alarm.checked?'1':'0',alive_seconds:String(seconds)})});dirty=false;paint(s);note.className='sub ok';note.textContent=`Saved ${s.gate.alive_seconds} seconds. ${s.gate.config_status}`}catch(e){note.className='sub err';note.textContent=`Not saved: ${e.message}`;dirty=true}finally{save.disabled=false}};refresh()</script>)HTML";
  statusServer.sendHeader("Cache-Control", "no-store, max-age=0");
  statusServer.send(200, "text/html; charset=utf-8", kPage);
}

void serveStatusJson() {
  String response;
  response.reserve(1000);
  response += "{\"uptime_ms\":";
  response += millis();
  response += ",\"bmp\":{\"ready\":";
  response += hasBmpReading ? "true" : "false";
  response += ",\"temperature_c\":";
  response += String(lastTemperatureC, 2);
  response += ",\"pressure_hpa\":";
  response += String(lastPressureHpa, 2);
  response += "},\"mcp_detected\":";
  response += mcpDetected ? "true" : "false";
  response += ",\"lora_healthy\":";
  response += loraHealthy ? "true" : "false";
  response += ",\"last_lora\":\"";
  response += jsonEscape(lastLoraReceive);
  response += "\",\"gate\":{\"state\":\"";
  response += gateState();
  response += "\",\"raw\":";
  response += gateRaw;
  response += ",\"alarm_enabled\":";
  response += gateAlarmEnabled ? "true" : "false";
  response += ",\"alarm_active\":";
  response += gateAlarmActive() ? "true" : "false";
  response += ",\"alive_seconds\":";
  response += gateAliveSeconds;
  response += ",\"last_kind\":\"";
  response += jsonEscape(gateLastKind);
  response += "\",\"last_packet_ms\":";
  response += gateLastPacketMs;
  response += ",\"last_sensor_sent_ms\":";
  response += gateLastSensorSentMs;
  response += ",\"estimated_transit_ms\":";
  response += gateEstimatedTransitMs;
  response += ",\"config_status\":\"";
  response += jsonEscape(gateConfigStatus);
  response += "\",\"history\":[";
  for (uint8_t index = 0; index < gateHistoryCount; ++index) {
    if (index > 0) response += ',';
    response += '"';
    response += jsonEscape(gateHistory[index]);
    response += '"';
  }
  response += "]}}";
  response.remove(response.length() - 1);
  response += ",\"incoming_log\":[";
  for (uint8_t index = 0; index < incomingLogCount; ++index) {
    if (index > 0) response += ',';
    response += '"';
    response += jsonEscape(incomingLog[index]);
    response += '"';
  }
  response += "]}";
  statusServer.sendHeader("Cache-Control", "no-store, max-age=0");
  statusServer.send(200, "application/json", response);
}

void serveGateCalibration() {
  if (gateRaw < 0) {
    statusServer.send(409, "application/json", "{\"error\":\"Awaiting Gate Sensor raw state\"}");
    return;
  }
  gateClosedRaw = gateRaw;
  ++gateConfigRevision;
  saveGateSettings();
  sendGateConfiguration();
  serveStatusJson();
}

void serveGateSettings() {
  const String alarm = statusServer.arg("alarm");
  const uint32_t requestedAlive = statusServer.arg("alive_seconds").toInt();
  if ((alarm != "0" && alarm != "1") || requestedAlive < kMinimumGateAliveSeconds ||
      requestedAlive > kMaximumGateAliveSeconds) {
    statusServer.send(400, "application/json", "{\"error\":\"Invalid gate settings\"}");
    return;
  }
  gateAlarmEnabled = alarm == "1";
  gateAliveSeconds = requestedAlive;
  ++gateConfigRevision;
  saveGateSettings();
  sendGateConfiguration();
  serveStatusJson();
}

void startStatusServer() {
  const IPAddress apIp(192, 168, 4, 1);
  const IPAddress gateway(192, 168, 4, 1);
  const IPAddress subnet(255, 255, 255, 0);
  // The Home Hub is mains powered and must remain continuously reachable.
  // Do not add ESP sleep modes here; disable Wi-Fi radio sleep as well.
  WiFi.mode(WIFI_AP);
  WiFi.setSleep(false);
  if (!WiFi.softAPConfig(apIp, gateway, subnet)) {
    Serial.println("Wi-Fi SoftAP network configuration failed");
    return;
  }
  if (!WiFi.softAP(kAccessPointSsid, nullptr, 1, false, 4)) {
    Serial.println("Wi-Fi SoftAP failed to start");
    return;
  }
  statusServer.on("/", HTTP_GET, serveResponsiveGateStatusPage);
  statusServer.on("/api/status", HTTP_GET, serveStatusJson);
  statusServer.on("/api/gate/settings", HTTP_POST, serveGateSettings);
  statusServer.onNotFound([]() {
    Serial.print("HTTP redirect from ");
    Serial.println(statusServer.uri());
    statusServer.sendHeader("Location", "http://192.168.4.1/", true);
    statusServer.send(302, "text/plain", "");
  });
  statusServer.begin();
  Serial.print("Wi-Fi SoftAP ready; browse to http://");
  Serial.println(WiFi.softAPIP());
}

int16_t readS16LE(const uint8_t* bytes) {
  return static_cast<int16_t>(readU16LE(bytes));
}

bool readRegisters(const uint8_t address, const uint8_t firstRegister,
                   uint8_t* values, const size_t valueCount) {
  Wire.beginTransmission(address);
  Wire.write(firstRegister);
  if (Wire.endTransmission(false) != 0) {
    return false;
  }

  const size_t received = Wire.requestFrom(static_cast<int>(address),
                                           static_cast<int>(valueCount));
  if (received != valueCount) {
    while (Wire.available()) {
      Wire.read();
    }
    return false;
  }

  for (size_t index = 0; index < valueCount; ++index) {
    values[index] = Wire.read();
  }
  return true;
}

void flashActivity() {
  activityUntilMs = millis() + kActivityFlashMs;
}

void updateIndicator() {
  const bool active = static_cast<int32_t>(activityUntilMs - millis()) > 0;
  ledcWrite(kIndicatorPin, active ? kLedActivityDuty : kLedIdleDuty);
}

bool probeI2cAddress(const uint8_t address) {
  Wire.beginTransmission(address);
  return Wire.endTransmission() == 0;
}

bool initializeBmp280() {
  uint8_t chipId = 0;
  if (!readRegisters(kBmp280Address, 0xD0, &chipId, 1)) {
    Serial.println("BMP280: no response at 0x76");
    return false;
  }

  Serial.print("BMP/BME sensor chip ID: 0x");
  Serial.println(chipId, HEX);
  if (chipId == kBme280ChipId) {
    Serial.println("BME280 found; this BMP280-only diagnostic will not read it.");
    return false;
  }
  if (chipId != kBmp280ChipId) {
    Serial.println("Unsupported pressure-sensor chip ID.");
    return false;
  }

  uint8_t calibration[24]{};
  if (!readRegisters(kBmp280Address, 0x88, calibration, sizeof(calibration))) {
    Serial.println("BMP280: calibration read failed");
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
    Serial.println("BMP280: invalid calibration data");
    return false;
  }

  // Temperature and pressure x1 oversampling, normal mode. This setting is
  // volatile and is required to produce fresh measurements.
  Wire.beginTransmission(kBmp280Address);
  Wire.write(0xF4);
  Wire.write(0x27);
  if (Wire.endTransmission() != 0) {
    Serial.println("BMP280: measurement-mode setup failed");
    return false;
  }

  Serial.println("BMP280: ready");
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
  pressureVariable2 += (pressureVariable1 * bmpCalibration.p5) << 17;
  pressureVariable2 += static_cast<int64_t>(bmpCalibration.p4) << 35;
  pressureVariable1 = ((pressureVariable1 * pressureVariable1 * bmpCalibration.p3) >> 8) +
                      ((pressureVariable1 * bmpCalibration.p2) << 12);
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
             (static_cast<int64_t>(bmpCalibration.p7) << 4);
  *pressureHpa = static_cast<float>(pressure) / 25600.0F;
  return true;
}

void requestLoraHealth() {
  while (Serial1.available()) {
    Serial1.read();
  }
  Serial1.print("AT\r\n");
  lastLoraHealthMs = millis();
  Serial.println("RYLR998: sent AT health check");
  flashActivity();
}

bool processGatePayload(const String& payload, const int sourceAddress) {
  // FS2|GATE|gate-01|sequence|EVENT-or-ALIVE|raw|OPEN-or-CLOSED|sensor_sent_ms
  String values[8];
  int valueIndex = 0;
  int start = 0;
  while (valueIndex < 8) {
    const int separator = payload.indexOf('|', start);
    values[valueIndex++] = separator < 0 ? payload.substring(start)
                                         : payload.substring(start, separator);
    if (separator < 0) {
      break;
    }
    start = separator + 1;
  }
  if (valueIndex == 8 && values[0] == "FS2" && values[1] == "GATE" &&
      values[2] == kGateSensorId && (values[5] == "0" || values[5] == "1")) {
    gateRadioAddress = sourceAddress;
    gateRaw = values[5] == "1" ? 1 : 0;
    gateLastKind = values[4];
    gateLastPacketMs = millis();
    gateLastSensorSentMs = values[7].toInt();
    const int64_t translatedSensorSent = static_cast<int64_t>(gateLastSensorSentMs) + gateSensorToHubOffsetMs;
    gateEstimatedTransitMs = translatedSensorSent > static_cast<int64_t>(gateLastPacketMs)
                                 ? 0 : static_cast<uint32_t>(gateLastPacketMs - translatedSensorSent);
    const String event = String(values[4]) + " #" + values[3] + " raw=" + values[5] +
                         " state=" + gateState() + " | sensor sent " + values[7] +
                         "ms | hub received " + String(gateLastPacketMs) + "ms | est radio " +
                         String(gateEstimatedTransitMs) + "ms";
    rememberGateEvent(event);
    rememberIncomingLog("Gate Sensor 01", event);
    if (gateLastKind == "EVENT") {
      Serial.print("Gate Sensor 01: ");
      Serial.println(event);
    }
    return true;
  }

  // FS2|SYNC|gate-01|hub_sent_ms|sensor_received_ms
  if (valueIndex == 5 && values[0] == "FS2" && values[1] == "SYNC" &&
      values[2] == kGateSensorId) {
    const int64_t hubSentMs = values[3].toInt();
    const int64_t sensorReceivedMs = values[4].toInt();
    const int64_t hubReceivedMs = millis();
    gateSensorToHubOffsetMs = ((hubSentMs + hubReceivedMs) / 2) - sensorReceivedMs;
    rememberIncomingLog("Gate Sensor 01", "Clock synchronized");
    return true;
  }

  // FS2|ACK|gate-01|revision|OK
  if (valueIndex == 5 && values[0] == "FS2" && values[1] == "ACK" &&
      values[2] == kGateSensorId && values[4] == "OK") {
    const uint32_t acknowledgedRevision = values[3].toInt();
    gateConfigStatus = acknowledgedRevision == gateConfigRevision
                            ? "Configuration acknowledged"
                            : "Received acknowledgement for an older configuration";
    rememberIncomingLog("Gate Sensor 01", "Configuration acknowledgement");
    Serial.println("Gate Sensor 01: configuration acknowledgement received");
    return true;
  }
  return false;
}

void processLoraLine(const String& line) {
  if (line == "+OK") {
    loraHealthy = true;
    Serial.println("RYLR998: UART health check passed");
  }
  if (line.startsWith("+RCV=")) {
    lastLoraReceive = line;
    const int firstComma = line.indexOf(',');
    const int secondComma = line.indexOf(',', firstComma + 1);
    const int thirdComma = line.indexOf(',', secondComma + 1);
    if (firstComma > 5 && secondComma > firstComma && thirdComma > secondComma) {
      const int sourceAddress = line.substring(5, firstComma).toInt();
      const String payload = line.substring(secondComma + 1, thirdComma);
      if (!processGatePayload(payload, sourceAddress)) {
        rememberIncomingLog("LoRa", payload);
      }
    }
  }
  flashActivity();
}

void pollLora() {
  while (Serial1.available()) {
    const char character = static_cast<char>(Serial1.read());
    if (character == '\r') {
      continue;
    }
    if (character == '\n') {
      if (!loraLine.isEmpty()) {
        processLoraLine(loraLine);
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
}  // namespace

void setup() {
  Serial.begin(115200);
  hubPreferences.begin("gate-hub", false);
  gateClosedRaw = kGateClosedRaw;
  gateAlarmEnabled = hubPreferences.getBool("alarm", true);
  gateAliveSeconds = hubPreferences.getUInt("alive_sec", kDefaultGateAliveSeconds);
  gateConfigRevision = hubPreferences.getUInt("cfg_rev", 0);
  if (gateAliveSeconds < kMinimumGateAliveSeconds ||
      gateAliveSeconds > kMaximumGateAliveSeconds) {
    gateAliveSeconds = kDefaultGateAliveSeconds;
  }
  ledcAttach(kIndicatorPin, kLedFrequencyHz, kLedResolutionBits);
  ledcWrite(kIndicatorPin, kLedIdleDuty);

  Wire.begin(kI2cSdaPin, kI2cSclPin);
  Wire.setTimeOut(50);
  Serial1.begin(kLoraBaudRate, SERIAL_8N1, kLoraRxPin, kLoraTxPin);
  delay(250);

  Serial.println();
  Serial.println("Farm Sense peripheral validation");
  bmpReady = initializeBmp280();
  requestLoraHealth();
  startStatusServer();
}

void loop() {
  const uint32_t nowMs = millis();
  pollLora();
  requestGateClockSync();
  statusServer.handleClient();
  updateIndicator();

  if (nowMs - lastSensorMs >= kSensorIntervalMs) {
    lastSensorMs = nowMs;
    float temperatureC = 0.0F;
    float pressureHpa = 0.0F;
    if (bmpReady && readBmp280(&temperatureC, &pressureHpa)) {
      lastTemperatureC = temperatureC;
      lastPressureHpa = pressureHpa;
      hasBmpReading = true;
      flashActivity();
    } else {
      Serial.println("BMP280: read failed");
    }
  }

  if (nowMs - lastMcpMs >= kMcpIntervalMs) {
    lastMcpMs = nowMs;
    const bool previousMcpDetected = mcpDetected;
    mcpDetected = probeI2cAddress(kMcp23017Address);
    if (mcpDetected != previousMcpDetected) {
      Serial.printf("MCP23017 at 0x20: %s\n", mcpDetected ? "DETECTED" : "NOT DETECTED");
    }
    if (mcpDetected) {
      flashActivity();
    }
  }

  if (!loraHealthy && nowMs - lastLoraHealthMs >= kLoraHealthRetryMs) {
    requestLoraHealth();
  }
}
