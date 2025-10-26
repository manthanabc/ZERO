#include "CalibrationStorage.h"
#include <stdio.h>

CalibrationStorage::CalibrationStorage() {}

void CalibrationStorage::loadDefaults(Settings& settings) {
  settings.deadzone_percent = 5.0f;
  settings.LX = { 470, 1960, 3845, 0 };
  settings.LY = {   0, 1927, 3180, 1 };
  settings.RX = { 302, 1975, 3900, 2 };
  settings.RY = { 170, 1940, 3538, 3 };
}

void CalibrationStorage::putAxisNVS(const char* prefix, const CalAxis& axis) {
  char key[12];
  snprintf(key, sizeof(key), "%smin", prefix);
  prefs.putInt(key, axis.min);
  snprintf(key, sizeof(key), "%sctr", prefix);
  prefs.putInt(key, axis.center);
  snprintf(key, sizeof(key), "%smax", prefix);
  prefs.putInt(key, axis.max);
  snprintf(key, sizeof(key), "%sidx", prefix);
  prefs.putInt(key, axis.index);
}

void CalibrationStorage::getAxisNVS(const char* prefix, CalAxis& axis, const CalAxis& defaultAxis) {
  char key[12];
  snprintf(key, sizeof(key), "%smin", prefix);
  axis.min = prefs.getInt(key, defaultAxis.min);
  snprintf(key, sizeof(key), "%sctr", prefix);
  axis.center = prefs.getInt(key, defaultAxis.center);
  snprintf(key, sizeof(key), "%smax", prefix);
  axis.max = prefs.getInt(key, defaultAxis.max);
  snprintf(key, sizeof(key), "%sidx", prefix);
  axis.index = prefs.getInt(key, defaultAxis.index);
}

void CalibrationStorage::saveSettings(const Settings& settings) {
  prefs.begin("cal", false);
  prefs.putFloat("dz", settings.deadzone_percent);
  putAxisNVS("LX_", settings.LX);
  putAxisNVS("LY_", settings.LY);
  putAxisNVS("RX_", settings.RX);
  putAxisNVS("RY_", settings.RY);
  prefs.end();
}

void CalibrationStorage::loadSettings(Settings& settings) {
  prefs.begin("cal", true);
  settings.deadzone_percent = prefs.getFloat("dz", 5.0f);
  getAxisNVS("LX_", settings.LX, {470, 1960, 3845, 0});
  getAxisNVS("LY_", settings.LY, {  0, 1927, 3180, 1});
  getAxisNVS("RX_", settings.RX, {302, 1975, 3900, 2});
  getAxisNVS("RY_", settings.RY, {170, 1940, 3538, 3});
  prefs.end();
}
