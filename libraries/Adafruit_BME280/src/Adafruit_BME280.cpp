#include "Adafruit_BME280.h"
#include "ORISimSensorHelpers.h"
#include <cmath>

Adafruit_BME280::Adafruit_BME280() : _addr(BME280_ADDRESS_ALTERNATE) {}
bool Adafruit_BME280::begin(uint8_t addr, TwoWire* theWire) { _addr = addr; if (theWire) theWire->begin(); return true; }
void Adafruit_BME280::setSampling(sensor_mode, sensor_sampling, sensor_sampling, sensor_sampling, sensor_filter, standby_duration) {}

static bool bmeGet(uint8_t addr, SimVirtualDevice* d) { return orisimFindByTypeAndAddress("BME280", addr, d); }
float Adafruit_BME280::readTemperature() { SimVirtualDevice d; return bmeGet(_addr, &d) ? orisimDeviceValue(d) : NAN; }
float Adafruit_BME280::readPressure() { SimVirtualDevice d; return bmeGet(_addr, &d) ? orisimDevicePressure(d) * 100.0f : NAN; }
float Adafruit_BME280::readHumidity() { SimVirtualDevice d; return bmeGet(_addr, &d) ? orisimDeviceHumidity(d) : NAN; }
float Adafruit_BME280::readAltitude(float seaLevelhPa) {
  float p = readPressure();
  if (std::isnan(p) || seaLevelhPa <= 0) return NAN;
  return 44330.0f * (1.0f - std::pow(p / 100.0f / seaLevelhPa, 0.1903f));
}
