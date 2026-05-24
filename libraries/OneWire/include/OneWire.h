#pragma once
#include "Arduino.h"

// Implemented in DallasTemperature.cpp. Kept as raw uint8_t* to avoid a circular include.
bool simDs18b20GetRawAddressAt(int index, uint8_t* addr);

class OneWire {
public:
  explicit OneWire(uint8_t pin) : pin(pin) {}

  void reset_search() {
    searchIndex = 0;
  }

  bool search(uint8_t* addr) {
    if (!addr) return false;

    if (!simDs18b20GetRawAddressAt(searchIndex, addr)) {
      return false;
    }

    searchIndex++;
    return true;
  }

private:
  uint8_t pin;
  int searchIndex = 0;
};
