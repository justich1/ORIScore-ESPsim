#pragma once
#include "Arduino.h"

class SoftwareSerial : public FakeSerial {
public:
  SoftwareSerial(int rxPin, int txPin, bool inverseLogic = false) {
    (void)rxPin; (void)txPin; (void)inverseLogic;
  }
};
