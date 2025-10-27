#ifndef INPUT_PROCESSOR_H
#define INPUT_PROCESSOR_H

#include "CalibrationModel.h"
#include <Arduino.h>

#ifndef XBOX_STICK_MAX
#define XBOX_STICK_MAX 32767
#endif

class InputProcessor {
public:
  InputProcessor();
  
  RawAxisData readRawAxes();
  NormalizedAxisData processAxes(const RawAxisData& raw, const Settings& settings);
  
  StickData getLeftStick() const;
  StickData getRightStick() const;

private:
  int16_t leftX, leftY, rightX, rightY;
  
  float normalizeFromCal(int raw, int minVal, int centerVal, int maxVal);
  float applyDeadzone(float normalized, float deadzonePercent);
  int16_t toStick(float normalized);
};

#endif
