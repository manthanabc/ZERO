#ifndef VIBRATION_HANDLER_H
#define VIBRATION_HANDLER_H

#include <XboxGamepadDevice.h>

class VibrationHandler {
public:
  VibrationHandler();
  
  void init();
  void handleVibration(XboxGamepadOutputReportData data);
  
  static VibrationHandler* getInstance();

private:
  static VibrationHandler* instance;
};

void OnVibrateEvent(XboxGamepadOutputReportData data);

#endif
