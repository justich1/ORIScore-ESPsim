#include "Adafruit_SHT31.h"
#include "ORISimSensorHelpers.h"

Adafruit_SHT31::Adafruit_SHT31(TwoWire* wire) : _wire(wire), _addr(SHT31_DEFAULT_ADDR), _heater(false) {}
bool Adafruit_SHT31::begin(uint8_t i2caddr) { _addr = i2caddr; if (_wire) _wire->begin(); return true; }
static bool shtGet(uint8_t addr, SimVirtualDevice* d) { return orisimFindByTypeAndAddress("SHT31", addr, d); }
float Adafruit_SHT31::readTemperature() { SimVirtualDevice d; return shtGet(_addr, &d) ? orisimDeviceValue(d) : NAN; }
float Adafruit_SHT31::readHumidity() { SimVirtualDevice d; return shtGet(_addr, &d) ? orisimDeviceHumidity(d) : NAN; }
void Adafruit_SHT31::heater(bool h) { _heater = h; }
bool Adafruit_SHT31::isHeaterEnabled() { return _heater; }
