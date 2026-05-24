#include "Adafruit_AHTX0.h"
#include "ORISimSensorHelpers.h"

Adafruit_AHTX0::Adafruit_AHTX0() : _addr(AHTX0_I2CADDR_DEFAULT), _begun(false) {}

bool Adafruit_AHTX0::begin(TwoWire* wire, int32_t sensor_id, uint8_t i2c_addr) {
  (void)sensor_id;
  _addr = i2c_addr;
  _begun = true;
  if (wire) wire->begin();
  return true;
}

bool Adafruit_AHTX0::getEvent(sensors_event_t* humidity, sensors_event_t* temp) {
  SimVirtualDevice d;
  if (!orisimFindByTypeAndAddress("AHT20", _addr, &d)) return false;
  if (humidity) { std::memset(humidity, 0, sizeof(sensors_event_t)); humidity->relative_humidity = orisimDeviceHumidity(d); humidity->timestamp = (int32_t)millis(); }
  if (temp) { std::memset(temp, 0, sizeof(sensors_event_t)); temp->temperature = orisimDeviceValue(d); temp->timestamp = (int32_t)millis(); }
  return true;
}
