#include "GamepadController.h"
#include "PinConfig.h"
#include <Arduino.h>

GamepadController::GamepadController() 
  : compositeHID("ESP32 Controller", "Mystfit", 100), gamepad(nullptr) {}

void GamepadController::init() {
  pinMode(PinConfig::buttonX, INPUT_PULLUP);
  pinMode(PinConfig::buttonY, INPUT_PULLUP);
  pinMode(PinConfig::buttonA, INPUT_PULLUP);
  pinMode(PinConfig::buttonB, INPUT_PULLUP);

  pinMode(PinConfig::startButton, INPUT_PULLUP);
  pinMode(PinConfig::selectButton, INPUT_PULLUP);

  pinMode(PinConfig::triggerL, INPUT_PULLUP);
  pinMode(PinConfig::triggerR, INPUT_PULLUP);
  pinMode(PinConfig::switchL, INPUT_PULLUP);
  pinMode(PinConfig::switchR, INPUT_PULLUP);

  pinMode(PinConfig::rightStickButton, INPUT_PULLUP);
  pinMode(PinConfig::leftStickButton, INPUT_PULLUP);

  pinMode(PinConfig::dUp, INPUT_PULLUP);
  pinMode(PinConfig::dDown, INPUT_PULLUP);
  pinMode(PinConfig::dRight, INPUT_PULLUP);
  pinMode(PinConfig::dLeft, INPUT_PULLUP);

  vibrationHandler.init();

  XboxSeriesXControllerDeviceConfiguration* config = new XboxSeriesXControllerDeviceConfiguration();
  BLEHostConfiguration hostConfig = config->getIdealHostConfiguration();

  gamepad = new XboxGamepadDevice(config);
  FunctionSlot<XboxGamepadOutputReportData> vibrationSlot(OnVibrateEvent);
  gamepad->onVibrate.attach(vibrationSlot);
  compositeHID.addDevice(gamepad);

  Serial.println("Starting composite HID device...");
  compositeHID.begin(hostConfig);
}

void GamepadController::updateButtons() {
  if (digitalRead(PinConfig::buttonA) == LOW) gamepad->press(XBOX_BUTTON_A); 
  else gamepad->release(XBOX_BUTTON_A);
  
  if (digitalRead(PinConfig::buttonB) == LOW) gamepad->press(XBOX_BUTTON_B); 
  else gamepad->release(XBOX_BUTTON_B);
  
  if (digitalRead(PinConfig::buttonX) == LOW) gamepad->press(XBOX_BUTTON_X); 
  else gamepad->release(XBOX_BUTTON_X);
  
  if (digitalRead(PinConfig::buttonY) == LOW) gamepad->press(XBOX_BUTTON_Y); 
  else gamepad->release(XBOX_BUTTON_Y);

  if (digitalRead(PinConfig::triggerL) == LOW) gamepad->press(XBOX_BUTTON_LB); 
  else gamepad->release(XBOX_BUTTON_LB);
  
  if (digitalRead(PinConfig::triggerR) == LOW) gamepad->press(XBOX_BUTTON_RB); 
  else gamepad->release(XBOX_BUTTON_RB);

  if (digitalRead(PinConfig::startButton) == LOW) gamepad->press(XBOX_BUTTON_START); 
  else gamepad->release(XBOX_BUTTON_START);
  
  if (digitalRead(PinConfig::selectButton) == LOW) gamepad->press(XBOX_BUTTON_SELECT); 
  else gamepad->release(XBOX_BUTTON_SELECT);

  if (digitalRead(PinConfig::switchL) == LOW) gamepad->setLeftTrigger(UINT16_MAX); 
  else gamepad->setLeftTrigger(0);
  
  if (digitalRead(PinConfig::switchR) == LOW) gamepad->setRightTrigger(UINT16_MAX); 
  else gamepad->setRightTrigger(0);

  if (digitalRead(PinConfig::rightStickButton) == LOW) gamepad->press(XBOX_BUTTON_RS); 
  else gamepad->release(XBOX_BUTTON_RS);
  
  if (digitalRead(PinConfig::leftStickButton) == LOW) gamepad->press(XBOX_BUTTON_LS); 
  else gamepad->release(XBOX_BUTTON_LS);

  if (digitalRead(PinConfig::dUp) == LOW) gamepad->pressDPadDirection(DPAD_UP);
  else if (digitalRead(PinConfig::dDown) == LOW) gamepad->pressDPadDirection(DPAD_DOWN);
  else if (digitalRead(PinConfig::dRight) == LOW) gamepad->pressDPadDirection(DPAD_RIGHT);
  else if (digitalRead(PinConfig::dLeft) == LOW) gamepad->pressDPadDirection(DPAD_LEFT);
  else gamepad->releaseDPad();
}

void GamepadController::updateThumbsticks(InputProcessor& processor, const Settings& settings) {
  RawAxisData raw = processor.readRawAxes();
  processor.processAxes(raw, settings);
  
  StickData left = processor.getLeftStick();
  StickData right = processor.getRightStick();
  
  gamepad->setLeftThumb(left.x, left.y);
  gamepad->setRightThumb(right.x, right.y);
}

void GamepadController::sendReport() {
  gamepad->sendGamepadReport();
}

bool GamepadController::isConnected() {
  return compositeHID.isConnected();
}
