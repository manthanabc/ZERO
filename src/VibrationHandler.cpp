#include "VibrationHandler.h"
#include "PinConfig.h"
#include <Arduino.h>

VibrationHandler* VibrationHandler::instance = nullptr;

VibrationHandler::VibrationHandler() {
  instance = this;
}

VibrationHandler* VibrationHandler::getInstance() {
  return instance;
}

void VibrationHandler::init() {
  pinMode(PinConfig::rumble, OUTPUT);
  pinMode(PinConfig::rumbleSecondary, OUTPUT);
  digitalWrite(PinConfig::rumbleSecondary, LOW);
}

void VibrationHandler::handleVibration(XboxGamepadOutputReportData data) {
  if (data.weakMotorMagnitude > 0 || data.strongMotorMagnitude > 0) {
    analogWrite(PinConfig::rumbleSecondary, data.weakMotorMagnitude);
    analogWrite(PinConfig::rumble, data.strongMotorMagnitude);
  } else {
    analogWrite(PinConfig::rumble, 0);
    analogWrite(PinConfig::rumbleSecondary, 0);
  }
}

void OnVibrateEvent(XboxGamepadOutputReportData data) {
  VibrationHandler* handler = VibrationHandler::getInstance();
  if (handler) {
    handler->handleVibration(data);
  }
}
