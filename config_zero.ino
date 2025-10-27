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

#include "PinConfig.h"
#include "CalibrationModel.h"
#include "CalibrationStorage.h"
#include "InputProcessor.h"
#include "GamepadController.h"
#include "VibrationHandler.h"
#include "WebServerAPI.h"

Settings settings;
CalibrationStorage calibrationStorage;
InputProcessor inputProcessor;
GamepadController gamepadController;
WebServerAPI* webServerAPI;

void setup() {
  Serial.begin(115200);
  delay(100);

  calibrationStorage.loadDefaults(settings);
  calibrationStorage.loadSettings(settings);

  gamepadController.init();

  webServerAPI = new WebServerAPI(settings, calibrationStorage, inputProcessor);
  webServerAPI->init();
  webServerAPI->begin();
}

void loop() {
  if (gamepadController.isConnected()) {
    gamepadController.updateButtons();
    gamepadController.updateThumbsticks(inputProcessor, settings);
    gamepadController.sendReport();
    delay(10);
  } else {
    delay(20);
  }
}
