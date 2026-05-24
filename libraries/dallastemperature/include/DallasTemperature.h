#pragma once
#include "Arduino.h"
#include "OneWire.h"

using DeviceAddress = uint8_t[8];
#define DEVICE_DISCONNECTED_C -127.0f

void simDs18b20SetTemp(const String& hexAddress, float temp, bool connected = true);
void simDs18b20Remove(const String& hexAddress);
void simDs18b20Clear();
bool simDs18b20GetRawAddressAt(int index, uint8_t* addr);

class DallasTemperature {
public:
  explicit DallasTemperature(OneWire* ow);
  void begin();
  void requestTemperatures();
  void setWaitForConversion(bool wait);
  int getDeviceCount();
  bool getAddress(DeviceAddress addr, int index);
  float getTempC(const DeviceAddress addr);
  float getTempCByIndex(int index);
  static bool validAddress(const DeviceAddress addr);

  // Compatibility helper for older tests.
  void setFakeDevice(const String& hexAddress, float temp);

private:
  OneWire* ow;
};
