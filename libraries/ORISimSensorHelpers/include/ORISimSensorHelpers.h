#pragma once

#include "Arduino.h"
#include <cstdlib>
#include <cmath>

inline String orisimTrimUpper(String s) {
  s.trim();
  s.toUpperCase();
  return s;
}

inline String orisimNormalizePin(String pin) {
  pin.trim();
  pin.toUpperCase();
  if (pin.startsWith("GPIO")) pin = pin.substring(4);
  if (pin == "A0") return String("0");
  return pin;
}

inline int orisimParseAddress(const String& text, int fallback = -1) {
  String s = text;
  s.trim();
  s.toUpperCase();
  if (!s.length()) return fallback;

  const char* c = s.c_str();
  char* end = nullptr;
  long v = 0;

  if (s.startsWith("0X")) v = std::strtol(c + 2, &end, 16);
  else v = std::strtol(c, &end, 10);

  if (end == c || v < 0 || v > 255) return fallback;
  return (int)v;
}

inline bool orisimAddressMatches(const String& actual, uint8_t expected) {
  int parsed = orisimParseAddress(actual, -1);
  return parsed < 0 || parsed == (int)expected;
}

inline bool orisimFindByTypeIndex(const char* type, int index, SimVirtualDevice* out) {
  return simVirtualHwGetByTypeIndex(String(type), index, out);
}

inline bool orisimFindFirstByType(const char* type, SimVirtualDevice* out) {
  return simVirtualHwGetByTypeIndex(String(type), 0, out);
}

inline bool orisimFindByTypeAndAddress(const char* type, uint8_t address, SimVirtualDevice* out) {
  int count = simVirtualHwCount(String(type));
  for (int i = 0; i < count; i++) {
    SimVirtualDevice d;
    if (!simVirtualHwGetByTypeIndex(String(type), i, &d)) continue;
    if (orisimAddressMatches(d.address, address)) {
      if (out) *out = d;
      return true;
    }
  }
  return false;
}

inline bool orisimFindByTypeName(const char* type, const char* name, SimVirtualDevice* out) {
  String wanted = String(name);
  int count = simVirtualHwCount(String(type));
  for (int i = 0; i < count; i++) {
    SimVirtualDevice d;
    if (!simVirtualHwGetByTypeIndex(String(type), i, &d)) continue;
    if (wanted.length() == 0 || d.name == wanted) {
      if (out) *out = d;
      return true;
    }
  }
  return false;
}

inline bool orisimFindByTypePin(const char* type, uint8_t pin, SimVirtualDevice* out) {
  String wanted = String((int)pin);
  int count = simVirtualHwCount(String(type));
  for (int i = 0; i < count; i++) {
    SimVirtualDevice d;
    if (!simVirtualHwGetByTypeIndex(String(type), i, &d)) continue;
    if (orisimNormalizePin(d.pin) == wanted) {
      if (out) *out = d;
      return true;
    }
  }
  return false;
}

inline float orisimFloat(const String& s, float fallback = NAN) {
  String v = s;
  v.trim();
  if (!v.length()) return fallback;
  return v.toFloat();
}

inline float orisimDeviceValue(const SimVirtualDevice& d, float fallback = NAN) {
  return orisimFloat(d.value, fallback);
}

inline float orisimDeviceHumidity(const SimVirtualDevice& d, float fallback = NAN) {
  return orisimFloat(d.humidity, fallback);
}

inline float orisimDevicePressure(const SimVirtualDevice& d, float fallback = NAN) {
  return orisimFloat(d.pressure, fallback);
}

inline int orisimDeviceBoolValue(const SimVirtualDevice& d, int fallback = LOW) {
  String v = orisimTrimUpper(d.value);
  if (v == "1" || v == "TRUE" || v == "ON" || v == "HIGH") return HIGH;
  if (v == "0" || v == "FALSE" || v == "OFF" || v == "LOW") return LOW;
  return fallback;
}

inline int orisimAnalogRawByTypeName(const char* type, const char* name, int fallback = -1) {
  SimVirtualDevice d;
  if (!orisimFindByTypeName(type, name, &d)) return fallback;
  String pin = orisimNormalizePin(d.pin);
  int gpio = pin.toInt();
  if (gpio < 0 || gpio > 255) return fallback;
  return simVirtualHwAnalogRaw((uint8_t)gpio);
}
