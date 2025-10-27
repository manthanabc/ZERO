#ifndef GAMEPAD_CONTROLLER_H
#define GAMEPAD_CONTROLLER_H

#include <XboxGamepadDevice.h>
#include <BleCompositeHID.h>
#include "CalibrationModel.h"
#include "InputProcessor.h"
#include "VibrationHandler.h"

class GamepadController {
public:
  GamepadController();
  
  void init();
  void updateButtons();
  void updateThumbsticks(InputProcessor& processor, const Settings& settings);
  void sendReport();
  bool isConnected();

private:
  XboxGamepadDevice* gamepad;
  BleCompositeHID compositeHID;
  VibrationHandler vibrationHandler;
};

#endif
