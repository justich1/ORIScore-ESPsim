#pragma once
#include "Arduino.h"

class ORISimFlowCounter {
public:
  explicit ORISimFlowCounter(uint8_t pin, float pulsesPerLiter = 450.0f);
  void begin();
  float getFrequencyHz();
  float getLitersPerMinute();
  float getPulsesPerLiter();
private:
  uint8_t _pin;
  float _pulsesPerLiter;
};
