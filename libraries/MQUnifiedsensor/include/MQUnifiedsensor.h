#pragma once
#include "Arduino.h"

class MQUnifiedsensor {
public:
  MQUnifiedsensor(String board, float voltageResolution, int adcBitResolution, uint8_t pin, String type);
  void setRegressionMethod(int method);
  void setA(float a);
  void setB(float b);
  void init();
  void update();
  float readSensor();
  float calibrate(float ratioCleanAir);
  void serialDebug();
private:
  String _type;
  uint8_t _pin;
  float _last;
};
