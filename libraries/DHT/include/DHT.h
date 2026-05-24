#pragma once

#include "Arduino.h"
#include <cmath>

// Kompatibilita s běžnou Adafruit DHT knihovnou.
#define DHT11 11
#define DHT12 12
#define DHT21 21
#define AM2301 21
#define DHT22 22
#define AM2302 22

// Hodně Arduino příkladů používá globální isnan(x).
// V MSVC/C++ je standardně ve std::isnan, proto ho tady zpřístupníme.
using std::isnan;

class DHT {
public:
  DHT(uint8_t pin, uint8_t type, uint8_t count = 6);

  void begin(uint8_t usec = 55);

  float readTemperature(bool isFahrenheit = false, bool force = false);
  float readHumidity(bool force = false);
  bool read(bool force = false);

  float convertCtoF(float c);
  float convertFtoC(float f);

  // Stejné rozhraní jako Adafruit DHT.
  float computeHeatIndex(bool isFahrenheit = true);
  static float computeHeatIndex(float temperature, float percentHumidity, bool isFahrenheit = true);

private:
  uint8_t _pin;
  uint8_t _type;
  uint8_t _count;
  uint8_t _pullTime;

  float _lastTemperatureC;
  float _lastHumidity;
  bool _lastResult;
  bool _begun;
  unsigned long _lastReadTime;
};
