
#include <Arduino.h>
#include <BleConnectionStatus.h>
#include <BleCompositeHID.h>
#include <XboxGamepadDevice.h>
#include <NimBLEDevice.h>

#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include <Preferences.h>
#include <inttypes.h>


//cors implementation in calibration server
static void addCORS(AsyncWebServerResponse *res) {
  res->addHeader("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
  res->addHeader("Access-Control-Allow-Headers", "Content-Type");
  // Optional but helpful for Chrome's Private Network Access preflight:
  res->addHeader("Access-Control-Allow-Private-Network", "true");
  // Cache the preflight for 10 min:
  res->addHeader("Access-Control-Max-Age", "600");
}

static void handleOptions(AsyncWebServerRequest *req) {
  AsyncWebServerResponse *res = req->beginResponse(204); // No Content
  req->send(res);
}


// ================== PIN MAP (unchanged) ==================
const int joyLX = 32;
const int joyLY = 33;
const int joyRX = 35;
const int joyRY = 34;

// Buttons
const int buttonX = 15;
const int buttonA = 18;
const int buttonB = 4;   // RX2
const int buttonY = 16;

const int dUp = 12;
const int ddown = 13;
const int dright = 27;
const int dleft = 14;

const int startButton  = 19;
const int selectButton = 21;

const int triggerL = 17; // LB
const int triggerR = 5;  // RB

// Misc
const int RUMBLE  = 22;
const int RUMBLES = 23;

const int RS = 1;
const int LS = 3;

const int switchL = 25; // TX2
const int switchR = 26; // D5





// ================== Buttons & Sticks ==================
int16_t joy1x = 0, joy1y = 0, joy2x = 0, joy2y = 0;


// ================== HID & Gamepad ==================
XboxGamepadDevice *gamepad;
BleCompositeHID compositeHID("ESP32 Controller", "Mystfit", 100);

// ================== Vibration Handler ==================
void OnVibrateEvent(XboxGamepadOutputReportData data) {
  if (data.weakMotorMagnitude > 0 || data.strongMotorMagnitude > 0) {
    analogWrite(RUMBLES, data.strongMotorMagnitude);
    analogWrite(RUMBLE,  data.weakMotorMagnitude);
  } else {
    analogWrite(RUMBLE,  0);
    analogWrite(RUMBLES, 0);
  }
}

// ================== Calibration Model + Storage ==================
struct CalAxis {
  int min;     // raw ADC min
  int center;  // raw ADC mid/center
  int max;     // raw ADC max
  int index;   // logical axis index (0..3)
};

struct Settings {
  float   deadzone_percent; // 0..50
  CalAxis LX, LY, RX, RY;
};

Preferences prefs;
Settings settings;

// Defaults from your constants
void loadDefaults() {
  settings.deadzone_percent = 5.0f;
  settings.LX = { 470, 1960, 3845, 0 };
  settings.LY = {   0, 1927, 3180, 1 };
  settings.RX = { 302, 1975, 3900, 2 };
  settings.RY = { 170, 1940, 3538, 3 };
}

static void putAxisNVS(const char* prefix, struct CalAxis& a) {
  char k[12];
  snprintf(k, sizeof(k), "%smin", prefix); prefs.putInt(k, a.min);
  snprintf(k, sizeof(k), "%sctr", prefix); prefs.putInt(k, a.center);
  snprintf(k, sizeof(k), "%smax", prefix); prefs.putInt(k, a.max);
  snprintf(k, sizeof(k), "%sidx", prefix); prefs.putInt(k, a.index);
}
static void getAxisNVS(const char* prefix, struct CalAxis& a, const struct CalAxis& def) {
  char k[12];
  snprintf(k, sizeof(k), "%smin", prefix); a.min    = prefs.getInt(k, def.min);
  snprintf(k, sizeof(k), "%sctr", prefix); a.center = prefs.getInt(k, def.center);
  snprintf(k, sizeof(k), "%smax", prefix); a.max    = prefs.getInt(k, def.max);
  snprintf(k, sizeof(k), "%sidx", prefix); a.index  = prefs.getInt(k, def.index);
}

void saveSettings() {
  prefs.begin("cal", false);
  prefs.putFloat("dz", settings.deadzone_percent);
  putAxisNVS("LX_", settings.LX);
  putAxisNVS("LY_", settings.LY);
  putAxisNVS("RX_", settings.RX);
  putAxisNVS("RY_", settings.RY);
  prefs.end();
}
void loadSettings() {
  prefs.begin("cal", true);
  settings.deadzone_percent = prefs.getFloat("dz", 5.0f);
  getAxisNVS("LX_", settings.LX, {470,1960,3845,0});
  getAxisNVS("LY_", settings.LY, {  0,1927,3180,1});
  getAxisNVS("RX_", settings.RX, {302,1975,3900,2});
  getAxisNVS("RY_", settings.RY, {170,1940,3538,3});
  prefs.end();
}

// ---------- Normalize / Deadzone / Stick helpers ----------
#ifndef XBOX_STICK_MAX
#define XBOX_STICK_MAX 32767
#endif

// Normalizes a raw ADC reading to [-1..+1] using explicit min/center/max.
static inline float normalizeFromCal_vals(int raw, int minV, int ctrV, int maxV) {
  if (minV >= maxV) return 0.0f;

  if (raw >= ctrV) {
    float span = (float)(maxV - ctrV);
    if (span <= 1.0f) span = 1.0f;
    return (float)(raw - ctrV) / span;      // 0..+1
  } else {
    float span = (float)(ctrV - minV);
    if (span <= 1.0f) span = 1.0f;
    return -(float)(ctrV - raw) / span;     // -1..0
  }
}
static inline float applyDeadzone(float norm, float dzPercent) {
  float dz = dzPercent * 0.01f;
  if (dz < 0.0f) dz = 0.0f; if (dz > 0.5f) dz = 0.5f;
  float a = fabsf(norm);
  if (a <= dz) return 0.0f;
  float out = (a - dz) / (1.0f - dz);
  return (norm < 0) ? -out : out;
}
static inline int16_t toStick(float norm) {
  float v = norm * (float)XBOX_STICK_MAX;
  if (v >  (float)XBOX_STICK_MAX) v =  (float)XBOX_STICK_MAX;
  if (v < -(float)XBOX_STICK_MAX) v = -(float)XBOX_STICK_MAX;
  return (int16_t)lroundf(v);
}

// ================== Wi-Fi + REST API ==================
AsyncWebServer server(80);
const char* SSID = "ESP32-Cal";
const char* PASS = "esp32cal123";

// Live cache for /raw
struct RawNorm { float LX, LY, RX, RY; } lastRaw = {0,0,0,0};

void handleval(AsyncWebServerRequest *req){
  testThumbstickss();
  StaticJsonDocument<256> doc;
  doc["LX"] = joy1x; doc["LY"] = joy1y; doc["RX"] = joy2x; doc["RY"] = joy2y;
  String out; serializeJson(doc, out);
  req->send(200, "application/json", out);
}
void handleGetSettings(AsyncWebServerRequest *req){
  StaticJsonDocument<512> doc;
  doc["deadzone_percent"] = settings.deadzone_percent;
  JsonObject axes = doc.createNestedObject("axes");
  JsonObject o;

  o = axes.createNestedObject("LX"); o["index"]=settings.LX.index; o["min"]=settings.LX.min; o["max"]=settings.LX.max; o["center"]=settings.LX.center;
  o = axes.createNestedObject("LY"); o["index"]=settings.LY.index; o["min"]=settings.LY.min; o["max"]=settings.LY.max; o["center"]=settings.LY.center;
  o = axes.createNestedObject("RX"); o["index"]=settings.RX.index; o["min"]=settings.RX.min; o["max"]=settings.RX.max; o["center"]=settings.RX.center;
  o = axes.createNestedObject("RY"); o["index"]=settings.RY.index; o["min"]=settings.RY.min; o["max"]=settings.RY.max; o["center"]=settings.RY.center;

  String out; serializeJson(doc, out);
  req->send(200, "application/json", out);
}

void handlePostSet(AsyncWebServerRequest *req, uint8_t *data, size_t len, size_t index, size_t total){
  static String body;
  if(index == 0) body = "";
  body.reserve(total);
  body += String((const char*)data).substring(0, len);
  if(index + len != total) return;

  StaticJsonDocument<1024> doc;
  DeserializationError err = deserializeJson(doc, body);
  if(err){ req->send(400, "application/json", "{\"ok\":false,\"err\":\"bad_json\"}"); return; }

  if (doc.containsKey("deadzone_percent")) settings.deadzone_percent = doc["deadzone_percent"].as<float>();

  JsonObject axes = doc["axes"];
  if (!axes.isNull()) {
    JsonObject ax;

    ax = axes["LX"]; if (!ax.isNull()) {
      if (ax.containsKey("index"))  settings.LX.index  = ax["index"].as<int>();
      if (ax.containsKey("min"))    settings.LX.min    = ax["min"].as<int>();
      if (ax.containsKey("max"))    settings.LX.max    = ax["max"].as<int>();
      if (ax.containsKey("center")) settings.LX.center = ax["center"].as<int>();
    }
    ax = axes["LY"]; if (!ax.isNull()) {
      if (ax.containsKey("index"))  settings.LY.index  = ax["index"].as<int>();
      if (ax.containsKey("min"))    settings.LY.min    = ax["min"].as<int>();
      if (ax.containsKey("max"))    settings.LY.max    = ax["max"].as<int>();
      if (ax.containsKey("center")) settings.LY.center = ax["center"].as<int>();
    }
    ax = axes["RX"]; if (!ax.isNull()) {
      if (ax.containsKey("index"))  settings.RX.index  = ax["index"].as<int>();
      if (ax.containsKey("min"))    settings.RX.min    = ax["min"].as<int>();
      if (ax.containsKey("max"))    settings.RX.max    = ax["max"].as<int>();
      if (ax.containsKey("center")) settings.RX.center = ax["center"].as<int>();
    }
    ax = axes["RY"]; if (!ax.isNull()) {
      if (ax.containsKey("index"))  settings.RY.index  = ax["index"].as<int>();
      if (ax.containsKey("min"))    settings.RY.min    = ax["min"].as<int>();
      if (ax.containsKey("max"))    settings.RY.max    = ax["max"].as<int>();
      if (ax.containsKey("center")) settings.RY.center = ax["center"].as<int>();
    }
  }

  saveSettings();
  req->send(200, "application/json", "{\"ok\":true}");
}

void handleGetRaw(AsyncWebServerRequest *req){
  int rLX = analogRead(joyLX);
  int rLY = analogRead(joyLY);
  int rRX = analogRead(joyRX);
  int rRY = analogRead(joyRY);

  // NOTE: /raw returns normalized **without** deadzone so calibration works
  lastRaw.LX = rLX;
  lastRaw.LY = rLY;
  lastRaw.RX = rRX;
  lastRaw.RY = rRY;

  StaticJsonDocument<256> doc;
  doc["LX"] = lastRaw.LX; doc["LY"] = lastRaw.LY; doc["RX"] = lastRaw.RX; doc["RY"] = lastRaw.RY;
  String out; serializeJson(doc, out);
  req->send(200, "application/json", out);
}


void testButtons() {
  if (digitalRead(buttonA) == LOW) gamepad->press(XBOX_BUTTON_A); else gamepad->release(XBOX_BUTTON_A);
  if (digitalRead(buttonB) == LOW) gamepad->press(XBOX_BUTTON_B); else gamepad->release(XBOX_BUTTON_B);
  if (digitalRead(buttonX) == LOW) gamepad->press(XBOX_BUTTON_X); else gamepad->release(XBOX_BUTTON_X);
  if (digitalRead(buttonY) == LOW) gamepad->press(XBOX_BUTTON_Y); else gamepad->release(XBOX_BUTTON_Y);

  if (digitalRead(triggerL) == LOW) gamepad->press(XBOX_BUTTON_LB); else gamepad->release(XBOX_BUTTON_LB);
  if (digitalRead(triggerR) == LOW) gamepad->press(XBOX_BUTTON_RB); else gamepad->release(XBOX_BUTTON_RB);

  if (digitalRead(startButton)  == LOW) gamepad->press(XBOX_BUTTON_START);  else gamepad->release(XBOX_BUTTON_START);
  if (digitalRead(selectButton) == LOW) gamepad->press(XBOX_BUTTON_SELECT); else gamepad->release(XBOX_BUTTON_SELECT);

  if (digitalRead(switchL) == LOW) gamepad->setLeftTrigger(UINT16_MAX); else gamepad->setLeftTrigger(0);
  if (digitalRead(switchR) == LOW) gamepad->setRightTrigger(UINT16_MAX); else gamepad->setRightTrigger(0);

  if (digitalRead(RS) == LOW) gamepad->press(XBOX_BUTTON_RS); else gamepad->release(XBOX_BUTTON_RS);
  if (digitalRead(LS) == LOW) gamepad->press(XBOX_BUTTON_LS); else gamepad->release(XBOX_BUTTON_LS);

  if (digitalRead(dUp) == LOW)      gamepad->pressDPadDirection(DPAD_UP);
  else if (digitalRead(ddown) == LOW)  gamepad->pressDPadDirection(DPAD_DOWN);
  else if (digitalRead(dright) == LOW) gamepad->pressDPadDirection(DPAD_RIGHT);
  else if (digitalRead(dleft) == LOW)  gamepad->pressDPadDirection(DPAD_LEFT);
  else gamepad->releaseDPad();
}

void testThumbstickss() {
  int rLX = analogRead(joyLX);
  int rLY = analogRead(joyLY);
  int rRX = analogRead(joyRX);
  int rRY = analogRead(joyRY);

  float nLX = normalizeFromCal_vals(rLX, settings.LX.min, settings.LX.center, settings.LX.max);
  float nLY = normalizeFromCal_vals(rLY, settings.LY.min, settings.LY.center, settings.LY.max);
  float nRX = normalizeFromCal_vals(rRX, settings.RX.min, settings.RX.center, settings.RX.max);
  float nRY = normalizeFromCal_vals(rRY, settings.RY.min, settings.RY.center, settings.RY.max);

  float dz = settings.deadzone_percent;
  nLX = applyDeadzone(nLX, dz);
  nLY = applyDeadzone(nLY, dz);
  nRX = applyDeadzone(nRX, dz);
  nRY = applyDeadzone(nRY, dz);

  joy1x = toStick(nLX);
  joy1y = toStick(nLY);
  joy2x = toStick(nRX);
  joy2y = toStick(nRY);

  gamepad->setLeftThumb( joy1x, joy1y );
  gamepad->setRightThumb( joy2x, joy2y );
}

void testThumbsticks() {
  testThumbstickss();

  gamepad->setLeftThumb( joy1x, joy1y );
  gamepad->setRightThumb( joy2x, joy2y );
}

// ================== Setup ==================
void setup() {
  Serial.begin(115200);
  delay(100);

  pinMode(buttonX, INPUT_PULLUP);
  pinMode(buttonY, INPUT_PULLUP);
  pinMode(buttonA, INPUT_PULLUP);
  pinMode(buttonB, INPUT_PULLUP);

  pinMode(startButton, INPUT_PULLUP);
  pinMode(selectButton, INPUT_PULLUP);

  pinMode(triggerL, INPUT_PULLUP);
  pinMode(triggerR, INPUT_PULLUP);
  pinMode(switchL, INPUT_PULLUP);
  pinMode(switchR, INPUT_PULLUP);

  pinMode(RS, INPUT_PULLUP);
  pinMode(LS, INPUT_PULLUP);

  pinMode(RUMBLE, OUTPUT);
  pinMode(RUMBLES, OUTPUT);
  digitalWrite(RUMBLE, LOW);
  digitalWrite(RUMBLES, LOW);

  pinMode(dUp, INPUT_PULLUP);
  pinMode(ddown, INPUT_PULLUP);
  pinMode(dright, INPUT_PULLUP);
  pinMode(dleft, INPUT_PULLUP);

  loadDefaults();
  loadSettings();

  // BLE HID config
  XboxSeriesXControllerDeviceConfiguration* config = new XboxSeriesXControllerDeviceConfiguration();
  BLEHostConfiguration hostConfig = config->getIdealHostConfiguration();

  gamepad = new XboxGamepadDevice(config);
  FunctionSlot<XboxGamepadOutputReportData> vibrationSlot(OnVibrateEvent);
  gamepad->onVibrate.attach(vibrationSlot);
  compositeHID.addDevice(gamepad);

  Serial.println("Starting composite HID device...");
  compositeHID.begin(hostConfig);

  // Wi-Fi AP + REST
  WiFi.mode(WIFI_AP);
  WiFi.softAP(SSID, PASS);
  Serial.printf("AP: %s  IP: %s\n", SSID, WiFi.softAPIP().toString().c_str());

  DefaultHeaders::Instance().addHeader("Access-Control-Allow-Origin", "*");
  DefaultHeaders::Instance().addHeader("Access-Control-Allow-Headers", "Content-Type");
  DefaultHeaders::Instance().addHeader("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
  DefaultHeaders::Instance().addHeader("Access-Control-Allow-Private-Network", "true");

  server.on("/settings", HTTP_GET, handleGetSettings);
  server.on("/set", HTTP_POST, [](AsyncWebServerRequest* req){}, NULL, handlePostSet);
  server.on("/raw", HTTP_GET, handleGetRaw);
  server.on("/val", HTTP_GET, handleval);

  // NEW: preflight handlers
  server.on("/settings", HTTP_OPTIONS, handleOptions);
  server.on("/set",       HTTP_OPTIONS, handleOptions);
  server.on("/raw",       HTTP_OPTIONS, handleOptions);
  server.on("/val",       HTTP_OPTIONS, handleOptions);
  server.begin();

}

// ================== Loop ==================
void loop() {
  if (compositeHID.isConnected()) {
    testButtons();
    testThumbsticks();
    gamepad->sendGamepadReport();
    delay(10);
  } else {
    delay(20);
  }
}
