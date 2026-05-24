#pragma once
#include "Arduino.h"
#include "Wire.h"

#define BMP085_ULTRALOWPOWER 0
#define BMP085_STANDARD 1
#define BMP085_HIGHRES 2
#define BMP085_ULTRAHIGHRES 3
#define BMP085_MODE_ULTRALOWPOWER 0
#define BMP085_MODE_STANDARD 1
#define BMP085_MODE_HIGHRES 2
#define BMP085_MODE_ULTRAHIGHRES 3

class Adafruit_BMP085 {
public:
  Adafruit_BMP085();
  bool begin(uint8_t mode = BMP085_STANDARD);
  float readTemperature();
  int32_t readPressure();
  float readAltitude(float sealevelPressure = 101325.0f);
  int32_t readSealevelPressure(float altitude_meters = 0.0f);
private:
  uint8_t _addr;
};

using Adafruit_BMP180 = Adafruit_BMP085;
