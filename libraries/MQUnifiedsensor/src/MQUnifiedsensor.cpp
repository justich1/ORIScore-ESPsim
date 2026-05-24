#include "MQUnifiedsensor.h"
#include "ORISimSensorHelpers.h"

MQUnifiedsensor::MQUnifiedsensor(String board, float voltageResolution, int adcBitResolution, uint8_t pin, String type)
  : _type(type), _pin(pin), _last(0.0f) { (void)board; (void)voltageResolution; (void)adcBitResolution; }
void MQUnifiedsensor::setRegressionMethod(int method) { (void)method; }
void MQUnifiedsensor::setA(float a) { (void)a; }
void MQUnifiedsensor::setB(float b) { (void)b; }
void MQUnifiedsensor::init() { pinMode(_pin, INPUT); }
void MQUnifiedsensor::update() { _last = readSensor(); }
float MQUnifiedsensor::readSensor() {
  SimVirtualDevice d;
  if (orisimFindByTypeName("MQ135", "mq135", &d) || orisimFindFirstByType("MQ135", &d)) return orisimDeviceValue(d, 0.0f);
  return (float)analogRead(_pin) / 1023.0f * 1000.0f;
}
float MQUnifiedsensor::calibrate(float ratioCleanAir) { (void)ratioCleanAir; return 1.0f; }
void MQUnifiedsensor::serialDebug() { Serial.println(String("MQUnifiedsensor ppm=") + String(readSensor(), 1)); }
