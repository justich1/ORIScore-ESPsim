#include "Adafruit_INA219.h"
#include "ORISimSensorHelpers.h"

Adafruit_INA219::Adafruit_INA219(uint8_t addr) : _addr(addr) {}
bool Adafruit_INA219::begin(TwoWire* theWire) { if (theWire) theWire->begin(); return true; }
void Adafruit_INA219::setCalibration_32V_2A() {}
void Adafruit_INA219::setCalibration_32V_1A() {}
void Adafruit_INA219::setCalibration_16V_400mA() {}
static bool inaGet(uint8_t addr, SimVirtualDevice* d) { return orisimFindByTypeAndAddress("INA219", addr, d); }
float Adafruit_INA219::getBusVoltage_V() { SimVirtualDevice d; return inaGet(_addr, &d) ? orisimDeviceValue(d) : NAN; }
float Adafruit_INA219::getCurrent_mA() { SimVirtualDevice d; return inaGet(_addr, &d) ? orisimDeviceHumidity(d) : NAN; }
float Adafruit_INA219::getPower_mW() { SimVirtualDevice d; return inaGet(_addr, &d) ? orisimDevicePressure(d) : NAN; }
float Adafruit_INA219::getShuntVoltage_mV() { return 0.0f; }
