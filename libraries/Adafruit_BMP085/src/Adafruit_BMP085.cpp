#include "Adafruit_BMP085.h"
#include "ORISimSensorHelpers.h"
#include <cmath>

Adafruit_BMP085::Adafruit_BMP085() : _addr(0x77) {}
bool Adafruit_BMP085::begin(uint8_t mode) { (void)mode; Wire.begin(); return true; }
static bool bmp180Get(SimVirtualDevice* d) { return orisimFindByTypeAndAddress("BMP180", 0x77, d); }
float Adafruit_BMP085::readTemperature() { SimVirtualDevice d; return bmp180Get(&d) ? orisimDeviceValue(d) : NAN; }
int32_t Adafruit_BMP085::readPressure() { SimVirtualDevice d; return bmp180Get(&d) ? (int32_t)(orisimDevicePressure(d) * 100.0f) : 0; }
float Adafruit_BMP085::readAltitude(float sealevelPressure) { float p = (float)readPressure(); if (p <= 0 || sealevelPressure <= 0) return NAN; return 44330.0f * (1.0f - std::pow(p / sealevelPressure, 0.1903f)); }
int32_t Adafruit_BMP085::readSealevelPressure(float altitude_meters) { float p = (float)readPressure(); if (p <= 0) return 0; return (int32_t)(p / std::pow(1.0f - altitude_meters / 44330.0f, 5.255f)); }
