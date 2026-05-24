#include "MQ135.h"
#include "ORISimSensorHelpers.h"

// DŮLEŽITÉ:
// Konstruktor se často volá jako globální objekt ještě před main()/setup().
// Nesmí tady být pinMode(), protože Arduino runtime mapy ještě nemusí být
// zkonstruované. Na Windows/MSVC to pak končilo 0xC0000005 ještě před startem
// firmware. Pin se nastaví až při prvním čtení, kdy už setup()/loop() běží.
MQ135::MQ135(uint8_t pin, float rzero) : _pin(pin), _rzero(rzero) {}

int MQ135::getRaw() {
  pinMode(_pin, INPUT);
  return analogRead(_pin);
}

float MQ135::getRZero() { return _rzero; }

float MQ135::getPPM() {
  SimVirtualDevice d;
  if (orisimFindByTypeName("MQ135", "mq135", &d) || orisimFindFirstByType("MQ135", &d)) {
    return orisimDeviceValue(d, 0.0f);
  }

  int raw = getRaw();
  return (float)raw / 1023.0f * 1000.0f;
}
