#ifndef WEB_SERVER_API_H
#define WEB_SERVER_API_H

#include <ESPAsyncWebServer.h>
#include "CalibrationModel.h"
#include "CalibrationStorage.h"
#include "InputProcessor.h"

class WebServerAPI {
public:
  WebServerAPI(Settings& settings, CalibrationStorage& storage, InputProcessor& processor);
  
  void init();
  void begin();

private:
  AsyncWebServer server;
  Settings& settings;
  CalibrationStorage& storage;
  InputProcessor& processor;
  
  static WebServerAPI* instance;
  
  void setupRoutes();
  void setupCORS();
  
  static void addCORS(AsyncWebServerResponse* response);
  static void handleOptions(AsyncWebServerRequest* request);
  
  static void handleGetRaw(AsyncWebServerRequest* request);
  static void handleGetVal(AsyncWebServerRequest* request);
  static void handleGetSettings(AsyncWebServerRequest* request);
  static void handlePostSet(AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total);
};

#endif
