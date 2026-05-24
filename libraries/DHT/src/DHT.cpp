#include "DHT.h"

#include <cmath>

static bool simIsDhtTypeCompatible(uint8_t configuredType, const String& virtualTypeIn) {
  String virtualType = virtualTypeIn;
  virtualType.trim();
  virtualType.toUpperCase();

  if (configuredType == DHT11) return virtualType == "DHT11";
  if (configuredType == DHT12) return virtualType == "DHT12";
  if (configuredType == DHT21) return virtualType == "DHT21" || virtualType == "AM2301";
  if (configuredType == DHT22) return virtualType == "DHT22" || virtualType == "AM2302";

  // Neznámý typ necháme projít, aby starší sketch s vlastním typem pořád šel ladit.
  return true;
}

static bool simFindAnyDhtByPin(uint8_t pin, SimVirtualDevice* out) {
  // Novější Arduino stub už má rychlou helper funkci.
  if (simVirtualHwFindDhtByPin(pin, out)) return true;

  // Fallback pro typy, které helper nemusí znát.
  const char* types[] = { "DHT11", "DHT12", "DHT21", "AM2301", "DHT22", "AM2302" };
  String pinText((int)pin);

  for (const char* type : types) {
    int count = simVirtualHwCount(String(type));
    for (int i = 0; i < count; i++) {
      SimVirtualDevice dev;
      if (simVirtualHwGetByTypeIndex(String(type), i, &dev) && dev.pin == pinText) {
        if (out) *out = dev;
        return true;
      }
    }
  }

  return false;
}

DHT::DHT(uint8_t pin, uint8_t type, uint8_t count)
  : _pin(pin),
    _type(type),
    _count(count),
    _pullTime(55),
    _lastTemperatureC(NAN),
    _lastHumidity(NAN),
    _lastResult(false),
    _begun(false),
    _lastReadTime(0) {
}

void DHT::begin(uint8_t usec) {
  _pullTime = usec;
  (void)_pullTime;
  (void)_count;

  pinMode(_pin, INPUT_PULLUP);
  _lastReadTime = 0;
  _lastTemperatureC = NAN;
  _lastHumidity = NAN;
  _lastResult = false;
  _begun = true;
}

bool DHT::read(bool force) {
  if (!_begun) begin();

  unsigned long now = millis();

  // Reálná DHT čidla se běžně nečtou častěji než cca 2 s.
  // Kvůli kompatibilitě vracíme poslední výsledek, pokud nejde o forced read.
  if (!force && _lastReadTime != 0 && (now - _lastReadTime) < 2000UL) {
    return _lastResult;
  }

  _lastReadTime = now;

  SimVirtualDevice dev;
  if (!simFindAnyDhtByPin(_pin, &dev) || !dev.connected) {
    _lastTemperatureC = NAN;
    _lastHumidity = NAN;
    _lastResult = false;
    return false;
  }

  if (!simIsDhtTypeCompatible(_type, dev.type)) {
    _lastTemperatureC = NAN;
    _lastHumidity = NAN;
    _lastResult = false;
    return false;
  }

  float t = dev.value.toFloat();
  float h = dev.humidity.toFloat();

  // DHT11 mívá menší rozsahy, DHT22/AM2302 širší. V simulátoru jen ořízneme nesmysly.
  if (h < 0.0f) h = 0.0f;
  if (h > 100.0f) h = 100.0f;

  _lastTemperatureC = t;
  _lastHumidity = h;
  _lastResult = true;
  return true;
}

float DHT::readHumidity(bool force) {
  if (!read(force)) return NAN;
  return _lastHumidity;
}

float DHT::readTemperature(bool isFahrenheit, bool force) {
  if (!read(force)) return NAN;
  return isFahrenheit ? convertCtoF(_lastTemperatureC) : _lastTemperatureC;
}

float DHT::convertCtoF(float c) {
  return c * 1.8f + 32.0f;
}

float DHT::convertFtoC(float f) {
  return (f - 32.0f) * 0.555555556f;
}

float DHT::computeHeatIndex(bool isFahrenheit) {
  float temperature = readTemperature(isFahrenheit);
  float humidity = readHumidity();

  if (std::isnan(temperature) || std::isnan(humidity)) return NAN;
  return computeHeatIndex(temperature, humidity, isFahrenheit);
}

float DHT::computeHeatIndex(float temperature, float percentHumidity, bool isFahrenheit) {
  // Vzorec z běžné Adafruit DHT knihovny. Interně počítá ve °F.
  float t = temperature;

  if (!isFahrenheit) {
    t = t * 1.8f + 32.0f;
  }

  float hi = 0.5f * (t + 61.0f + ((t - 68.0f) * 1.2f) + (percentHumidity * 0.094f));

  if (hi > 79.0f) {
    hi = -42.379f +
         2.04901523f * t +
         10.14333127f * percentHumidity +
         -0.22475541f * t * percentHumidity +
         -0.00683783f * t * t +
         -0.05481717f * percentHumidity * percentHumidity +
         0.00122874f * t * t * percentHumidity +
         0.00085282f * t * percentHumidity * percentHumidity +
         -0.00000199f * t * t * percentHumidity * percentHumidity;

    if ((percentHumidity < 13.0f) && (t >= 80.0f) && (t <= 112.0f)) {
      hi -= ((13.0f - percentHumidity) * 0.25f) * std::sqrt((17.0f - std::fabs(t - 95.0f)) * 0.05882f);
    }
    else if ((percentHumidity > 85.0f) && (t >= 80.0f) && (t <= 87.0f)) {
      hi += ((percentHumidity - 85.0f) * 0.1f) * ((87.0f - t) * 0.2f);
    }
  }

  if (!isFahrenheit) {
    hi = (hi - 32.0f) * 0.555555556f;
  }

  return hi;
}
