#pragma once
#include "Arduino.h"
#include "Wire.h"
#include "Adafruit_Sensor.h"

#define AHTX0_I2CADDR_DEFAULT 0x38

class Adafruit_AHTX0 {
public:
  Adafruit_AHTX0();
  bool begin(TwoWire* wire = &Wire, int32_t sensor_id = 0, uint8_t i2c_addr = AHTX0_I2CADDR_DEFAULT);
  bool getEvent(sensors_event_t* humidity, sensors_event_t* temp);
private:
  uint8_t _addr;
  bool _begun;
};
