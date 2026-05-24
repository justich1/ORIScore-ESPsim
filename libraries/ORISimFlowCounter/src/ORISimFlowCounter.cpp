#include "ORISimFlowCounter.h"
#include "ORISimSensorHelpers.h"

ORISimFlowCounter::ORISimFlowCounter(uint8_t pin, float pulsesPerLiter) : _pin(pin), _pulsesPerLiter(pulsesPerLiter) {}
void ORISimFlowCounter::begin() { pinMode(_pin, INPUT_PULLUP); }
float ORISimFlowCounter::getPulsesPerLiter() { return _pulsesPerLiter; }
float ORISimFlowCounter::getFrequencyHz() {
  SimVirtualDevice d;
  if (orisimFindByTypePin("PULSE_COUNTER", _pin, &d) || orisimFindFirstByType("PULSE_COUNTER", &d)) {
    float ppl = orisimDeviceHumidity(d, _pulsesPerLiter);
    if (ppl > 0) _pulsesPerLiter = ppl;
    return orisimDeviceValue(d, 0.0f);
  }
  return 0.0f;
}
float ORISimFlowCounter::getLitersPerMinute() {
  float hz = getFrequencyHz();
  if (_pulsesPerLiter <= 0.0f) return 0.0f;
  return hz * 60.0f / _pulsesPerLiter;
}
