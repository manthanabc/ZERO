#include "InputProcessor.h"
#include "PinConfig.h"
#include <math.h>

InputProcessor::InputProcessor() 
  : leftX(0), leftY(0), rightX(0), rightY(0) {}

RawAxisData InputProcessor::readRawAxes() {
  RawAxisData raw;
  raw.LX = analogRead(PinConfig::joyLX);
  raw.LY = analogRead(PinConfig::joyLY);
  raw.RX = analogRead(PinConfig::joyRX);
  raw.RY = analogRead(PinConfig::joyRY);
  return raw;
}

float InputProcessor::normalizeFromCal(int raw, int minVal, int centerVal, int maxVal) {
  if (minVal >= maxVal) return 0.0f;

  if (raw >= centerVal) {
    float span = (float)(maxVal - centerVal);
    if (span <= 1.0f) span = 1.0f;
    return (float)(raw - centerVal) / span;
  } else {
    float span = (float)(centerVal - minVal);
    if (span <= 1.0f) span = 1.0f;
    return -(float)(centerVal - raw) / span;
  }
}

float InputProcessor::applyDeadzone(float normalized, float deadzonePercent) {
  float dz = deadzonePercent * 0.01f;
  if (dz < 0.0f) dz = 0.0f;
  if (dz > 0.5f) dz = 0.5f;
  
  float absVal = fabsf(normalized);
  if (absVal <= dz) return 0.0f;
  
  float output = (absVal - dz) / (1.0f - dz);
  return (normalized < 0) ? -output : output;
}

int16_t InputProcessor::toStick(float normalized) {
  float value = normalized * (float)XBOX_STICK_MAX;
  if (value >  (float)XBOX_STICK_MAX) value =  (float)XBOX_STICK_MAX;
  if (value < -(float)XBOX_STICK_MAX) value = -(float)XBOX_STICK_MAX;
  return (int16_t)lroundf(value);
}

NormalizedAxisData InputProcessor::processAxes(const RawAxisData& raw, const Settings& settings) {
  float nLX = normalizeFromCal(raw.LX, settings.LX.min, settings.LX.center, settings.LX.max);
  float nLY = normalizeFromCal(raw.LY, settings.LY.min, settings.LY.center, settings.LY.max);
  float nRX = normalizeFromCal(raw.RX, settings.RX.min, settings.RX.center, settings.RX.max);
  float nRY = normalizeFromCal(raw.RY, settings.RY.min, settings.RY.center, settings.RY.max);

  float dz = settings.deadzone_percent;
  nLX = applyDeadzone(nLX, dz);
  nLY = applyDeadzone(nLY, dz);
  nRX = applyDeadzone(nRX, dz);
  nRY = applyDeadzone(nRY, dz);

  leftX = toStick(nLX);
  leftY = toStick(nLY);
  rightX = toStick(nRX);
  rightY = toStick(nRY);

  NormalizedAxisData result;
  result.LX = nLX;
  result.LY = nLY;
  result.RX = nRX;
  result.RY = nRY;
  return result;
}

StickData InputProcessor::getLeftStick() const {
  return { leftX, leftY };
}

StickData InputProcessor::getRightStick() const {
  return { rightX, rightY };
}
