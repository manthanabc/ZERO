#ifndef CALIBRATION_MODEL_H
#define CALIBRATION_MODEL_H

#include <stdint.h>

struct CalAxis {
  int min;
  int center;
  int max;
  int index;
};

struct Settings {
  float deadzone_percent;
  CalAxis LX, LY, RX, RY;
};

struct RawAxisData {
  int LX, LY, RX, RY;
};

struct NormalizedAxisData {
  float LX, LY, RX, RY;
};

struct StickData {
  int16_t x, y;
};

#endif
