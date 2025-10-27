#ifndef CALIBRATION_STORAGE_H
#define CALIBRATION_STORAGE_H

#include "CalibrationModel.h"
#include <Preferences.h>

class CalibrationStorage {
public:
  CalibrationStorage();
  
  void loadDefaults(Settings& settings);
  void loadSettings(Settings& settings);
  void saveSettings(const Settings& settings);

private:
  Preferences prefs;
  
  void putAxisNVS(const char* prefix, const CalAxis& axis);
  void getAxisNVS(const char* prefix, CalAxis& axis, const CalAxis& defaultAxis);
};

#endif
