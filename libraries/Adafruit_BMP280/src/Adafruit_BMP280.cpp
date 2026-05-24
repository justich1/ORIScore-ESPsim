#include "Adafruit_BMP280.h"
#include "ORISimSensorHelpers.h"
#include <cmath>

Adafruit_BMP280::Adafruit_BMP280() : _addr(BMP280_ADDRESS_ALT) {}
bool Adafruit_BMP280::begin(uint8_t addr, uint8_t chipid) { (void)chipid; _addr = addr; Wire.begin(); return true; }
bool Adafruit_BMP280::begin(uint8_t addr, TwoWire* theWire) { _addr = addr; if (theWire) theWire->begin(); return true; }
void Adafruit_BMP280::setSampling(sensor_mode, sensor_sampling, sensor_sampling, sensor_filter, standby_duration) {}
static bool bmpGet(uint8_t addr, SimVirtualDevice* d) { return orisimFindByTypeAndAddress("BMP280", addr, d); }
float Adafruit_BMP280::readTemperature() { SimVirtualDevice d; return bmpGet(_addr, &d) ? orisimDeviceValue(d) : NAN; }
float Adafruit_BMP280::readPressure() { SimVirtualDevice d; return bmpGet(_addr, &d) ? orisimDevicePressure(d) * 100.0f : NAN; }
float Adafruit_BMP280::readAltitude(float seaLevelhPa) { float p = readPressure(); if (std::isnan(p) || seaLevelhPa <= 0) return NAN; return 44330.0f * (1.0f - std::pow(p / 100.0f / seaLevelhPa, 0.1903f)); }
