#pragma once
#include "Arduino.h"
#include "Wire.h"

#define SHT31_DEFAULT_ADDR 0x44

class Adafruit_SHT31 {
public:
  Adafruit_SHT31(TwoWire* wire = &Wire);
  bool begin(uint8_t i2caddr = SHT31_DEFAULT_ADDR);
  float readTemperature();
  float readHumidity();
  void heater(bool h);
  bool isHeaterEnabled();
private:
  TwoWire* _wire;
  uint8_t _addr;
  bool _heater;
};
