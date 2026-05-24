#pragma once
#include "Arduino.h"
#include <vector>

class TwoWire {
public:
  TwoWire() = default;
  explicit TwoWire(int bus) : _bus(bus) {}

  void begin() {
    Serial.println("[SIM] Wire.begin");
  }

  void begin(int sda, int scl) {
    _sda = sda;
    _scl = scl;
    Serial.println(String("[SIM] Wire.begin SDA=") + String(sda) + " SCL=" + String(scl));
  }

  void setClock(uint32_t clock) {
    _clock = clock;
  }

  void beginTransmission(uint8_t address) {
    _address = address;
    _tx.clear();
  }

  size_t write(uint8_t b) {
    _tx.push_back(b);
    return 1;
  }

  size_t write(const uint8_t* data, size_t len) {
    if (!data) return 0;
    for (size_t i = 0; i < len; i++) _tx.push_back(data[i]);
    return len;
  }

  uint8_t endTransmission(bool stopBit = true) {
    (void)stopBit;
    Serial.println(String("[SIM] Wire.endTransmission addr=0x") + String((int)_address, HEX) +
                   " bytes=" + String((int)_tx.size()));
    return 0;
  }

  uint8_t requestFrom(uint8_t address, uint8_t quantity) {
    _address = address;
    _rx.clear();
    for (uint8_t i = 0; i < quantity; i++) _rx.push_back(0);
    return quantity;
  }

  int available() {
    return (int)_rx.size();
  }

  int read() {
    if (_rx.empty()) return -1;
    uint8_t b = _rx.front();
    _rx.erase(_rx.begin());
    return b;
  }

private:
  int _bus = 0;
  int _sda = -1;
  int _scl = -1;
  uint32_t _clock = 100000;
  uint8_t _address = 0;
  std::vector<uint8_t> _tx;
  std::vector<uint8_t> _rx;
};

extern TwoWire Wire;
