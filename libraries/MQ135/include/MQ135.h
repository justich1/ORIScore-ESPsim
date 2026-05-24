#pragma once
#include "Arduino.h"

class MQ135 {
public:
  explicit MQ135(uint8_t pin, float rzero = 76.63f);
  float getPPM();
  float getRZero();
  int getRaw();
private:
  uint8_t _pin;
  float _rzero;
};
