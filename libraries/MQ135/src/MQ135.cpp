#include "MQ135.h"
#include "ORISimSensorHelpers.h"

MQ135::MQ135(uint8_t pin, float rzero) : _pin(pin), _rzero(rzero) { pinMode(pin, INPUT); }
int MQ135::getRaw() { return analogRead(_pin); }
float MQ135::getRZero() { return _rzero; }
float MQ135::getPPM() {
  SimVirtualDevice d;
  if (orisimFindByTypeName("MQ135", "mq135", &d) || orisimFindFirstByType("MQ135", &d)) return orisimDeviceValue(d, 0.0f);
  int raw = getRaw();
  return (float)raw / 1023.0f * 1000.0f;
}
