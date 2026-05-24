#pragma once
#include "Arduino.h"
#include "Wire.h"

#define BMP280_ADDRESS                0x77
#define BMP280_ADDRESS_ALT            0x76

class Adafruit_BMP280 {
public:
  enum sensor_sampling { SAMPLING_NONE = 0, SAMPLING_X1 = 1, SAMPLING_X2 = 2, SAMPLING_X4 = 3, SAMPLING_X8 = 4, SAMPLING_X16 = 5 };
  enum sensor_mode { MODE_SLEEP = 0, MODE_FORCED = 1, MODE_NORMAL = 3 };
  enum sensor_filter { FILTER_OFF = 0, FILTER_X2 = 1, FILTER_X4 = 2, FILTER_X8 = 3, FILTER_X16 = 4 };
  enum standby_duration { STANDBY_MS_1 = 0, STANDBY_MS_63 = 1, STANDBY_MS_125 = 2, STANDBY_MS_250 = 3, STANDBY_MS_500 = 4, STANDBY_MS_1000 = 5, STANDBY_MS_2000 = 6, STANDBY_MS_4000 = 7 };

  Adafruit_BMP280();
  bool begin(uint8_t addr = BMP280_ADDRESS_ALT, uint8_t chipid = 0x58);
  bool begin(uint8_t addr, TwoWire* theWire);
  void setSampling(sensor_mode mode = MODE_NORMAL, sensor_sampling tempSampling = SAMPLING_X16,
                   sensor_sampling pressSampling = SAMPLING_X16, sensor_filter filter = FILTER_OFF,
                   standby_duration duration = STANDBY_MS_1);
  float readTemperature();
  float readPressure();
  float readAltitude(float seaLevelhPa = 1013.25f);
private:
  uint8_t _addr;
};
