#include "WebServerAPI.h"
#include "PinConfig.h"
#include <WiFi.h>
#include <ArduinoJson.h>

WebServerAPI* WebServerAPI::instance = nullptr;

WebServerAPI::WebServerAPI(Settings& settings, CalibrationStorage& storage, InputProcessor& processor)
  : server(80), settings(settings), storage(storage), processor(processor) {
  instance = this;
}

void WebServerAPI::init() {
  const char* SSID = "ESP32-Cal";
  const char* PASS = "esp32cal123";
  
  WiFi.mode(WIFI_AP);
  WiFi.softAP(SSID, PASS);
  Serial.printf("AP: %s  IP: %s\n", SSID, WiFi.softAPIP().toString().c_str());
  
  setupCORS();
  setupRoutes();
}

void WebServerAPI::setupCORS() {
  DefaultHeaders::Instance().addHeader("Access-Control-Allow-Origin", "*");
  DefaultHeaders::Instance().addHeader("Access-Control-Allow-Headers", "Content-Type");
  DefaultHeaders::Instance().addHeader("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
  DefaultHeaders::Instance().addHeader("Access-Control-Allow-Private-Network", "true");
}

void WebServerAPI::setupRoutes() {
  server.on("/settings", HTTP_GET, handleGetSettings);
  server.on("/set", HTTP_POST, [](AsyncWebServerRequest* req){}, NULL, handlePostSet);
  server.on("/raw", HTTP_GET, handleGetRaw);
  server.on("/val", HTTP_GET, handleGetVal);

  server.on("/settings", HTTP_OPTIONS, handleOptions);
  server.on("/set", HTTP_OPTIONS, handleOptions);
  server.on("/raw", HTTP_OPTIONS, handleOptions);
  server.on("/val", HTTP_OPTIONS, handleOptions);
}

void WebServerAPI::begin() {
  server.begin();
}

void WebServerAPI::addCORS(AsyncWebServerResponse* response) {
  response->addHeader("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
  response->addHeader("Access-Control-Allow-Headers", "Content-Type");
  response->addHeader("Access-Control-Allow-Private-Network", "true");
  response->addHeader("Access-Control-Max-Age", "600");
}

void WebServerAPI::handleOptions(AsyncWebServerRequest* request) {
  AsyncWebServerResponse* response = request->beginResponse(204);
  request->send(response);
}

void WebServerAPI::handleGetRaw(AsyncWebServerRequest* request) {
  if (!instance) {
    request->send(500, "application/json", "{\"error\":\"server not initialized\"}");
    return;
  }
  
  int rLX = analogRead(PinConfig::joyLX);
  int rLY = analogRead(PinConfig::joyLY);
  int rRX = analogRead(PinConfig::joyRX);
  int rRY = analogRead(PinConfig::joyRY);

  StaticJsonDocument<256> doc;
  doc["LX"] = rLX;
  doc["LY"] = rLY;
  doc["RX"] = rRX;
  doc["RY"] = rRY;
  
  String output;
  serializeJson(doc, output);
  request->send(200, "application/json", output);
}

void WebServerAPI::handleGetVal(AsyncWebServerRequest* request) {
  if (!instance) {
    request->send(500, "application/json", "{\"error\":\"server not initialized\"}");
    return;
  }
  
  RawAxisData raw = instance->processor.readRawAxes();
  instance->processor.processAxes(raw, instance->settings);
  
  StickData left = instance->processor.getLeftStick();
  StickData right = instance->processor.getRightStick();
  
  StaticJsonDocument<256> doc;
  doc["LX"] = left.x;
  doc["LY"] = left.y;
  doc["RX"] = right.x;
  doc["RY"] = right.y;
  
  String output;
  serializeJson(doc, output);
  request->send(200, "application/json", output);
}

void WebServerAPI::handleGetSettings(AsyncWebServerRequest* request) {
  if (!instance) {
    request->send(500, "application/json", "{\"error\":\"server not initialized\"}");
    return;
  }
  
  StaticJsonDocument<512> doc;
  doc["deadzone_percent"] = instance->settings.deadzone_percent;
  
  JsonObject axes = doc.createNestedObject("axes");
  JsonObject obj;

  obj = axes.createNestedObject("LX");
  obj["index"] = instance->settings.LX.index;
  obj["min"] = instance->settings.LX.min;
  obj["max"] = instance->settings.LX.max;
  obj["center"] = instance->settings.LX.center;

  obj = axes.createNestedObject("LY");
  obj["index"] = instance->settings.LY.index;
  obj["min"] = instance->settings.LY.min;
  obj["max"] = instance->settings.LY.max;
  obj["center"] = instance->settings.LY.center;

  obj = axes.createNestedObject("RX");
  obj["index"] = instance->settings.RX.index;
  obj["min"] = instance->settings.RX.min;
  obj["max"] = instance->settings.RX.max;
  obj["center"] = instance->settings.RX.center;

  obj = axes.createNestedObject("RY");
  obj["index"] = instance->settings.RY.index;
  obj["min"] = instance->settings.RY.min;
  obj["max"] = instance->settings.RY.max;
  obj["center"] = instance->settings.RY.center;

  String output;
  serializeJson(doc, output);
  request->send(200, "application/json", output);
}

void WebServerAPI::handlePostSet(AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) {
  if (!instance) {
    request->send(500, "application/json", "{\"ok\":false,\"err\":\"server not initialized\"}");
    return;
  }
  
  static String body;
  if (index == 0) body = "";
  body.reserve(total);
  body += String((const char*)data).substring(0, len);
  if (index + len != total) return;

  StaticJsonDocument<1024> doc;
  DeserializationError err = deserializeJson(doc, body);
  if (err) {
    request->send(400, "application/json", "{\"ok\":false,\"err\":\"bad_json\"}");
    return;
  }

  if (doc.containsKey("deadzone_percent")) {
    instance->settings.deadzone_percent = doc["deadzone_percent"].as<float>();
  }

  JsonObject axes = doc["axes"];
  if (!axes.isNull()) {
    JsonObject ax;

    ax = axes["LX"];
    if (!ax.isNull()) {
      if (ax.containsKey("index")) instance->settings.LX.index = ax["index"].as<int>();
      if (ax.containsKey("min")) instance->settings.LX.min = ax["min"].as<int>();
      if (ax.containsKey("max")) instance->settings.LX.max = ax["max"].as<int>();
      if (ax.containsKey("center")) instance->settings.LX.center = ax["center"].as<int>();
    }

    ax = axes["LY"];
    if (!ax.isNull()) {
      if (ax.containsKey("index")) instance->settings.LY.index = ax["index"].as<int>();
      if (ax.containsKey("min")) instance->settings.LY.min = ax["min"].as<int>();
      if (ax.containsKey("max")) instance->settings.LY.max = ax["max"].as<int>();
      if (ax.containsKey("center")) instance->settings.LY.center = ax["center"].as<int>();
    }

    ax = axes["RX"];
    if (!ax.isNull()) {
      if (ax.containsKey("index")) instance->settings.RX.index = ax["index"].as<int>();
      if (ax.containsKey("min")) instance->settings.RX.min = ax["min"].as<int>();
      if (ax.containsKey("max")) instance->settings.RX.max = ax["max"].as<int>();
      if (ax.containsKey("center")) instance->settings.RX.center = ax["center"].as<int>();
    }

    ax = axes["RY"];
    if (!ax.isNull()) {
      if (ax.containsKey("index")) instance->settings.RY.index = ax["index"].as<int>();
      if (ax.containsKey("min")) instance->settings.RY.min = ax["min"].as<int>();
      if (ax.containsKey("max")) instance->settings.RY.max = ax["max"].as<int>();
      if (ax.containsKey("center")) instance->settings.RY.center = ax["center"].as<int>();
    }
  }

  instance->storage.saveSettings(instance->settings);
  request->send(200, "application/json", "{\"ok\":true}");
}
