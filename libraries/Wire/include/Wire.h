#pragma once

#include "Arduino.h"

#include <cstddef>
#include <cstdint>
#include <vector>

class TwoWire {
public:
  TwoWire();
  explicit TwoWire(int bus);

  void begin();
  void begin(int sda, int scl);
  void setClock(uint32_t clock);

  void beginTransmission(uint8_t address);
  size_t write(uint8_t b);
  size_t write(const uint8_t* data, size_t len);
  uint8_t endTransmission(bool stopBit = true);

  uint8_t requestFrom(uint8_t address, uint8_t quantity);
  size_t requestFrom(int address, int quantity);

  int available();
  int read();

private:
  void ensureRtcInitialized();
  void updateRtcRegisters();
  void applyRtcWrite(uint8_t firstRegister, const uint8_t* data, size_t length);
  void setRtcFromRegisters();
  void loadRtcState();
  void saveRtcState(bool force);

  int _bus = 0;
  int _sda = -1;
  int _scl = -1;
  uint32_t _clock = 100000;
  uint8_t _address = 0;
  uint8_t _registerPointer = 0;
  std::vector<uint8_t> _tx;
  std::vector<uint8_t> _rx;

  // PCF8563/BM8563 simulator at I2C address 0x51.
  bool _rtcInitialized = false;
  bool _rtcRunning = true;
  bool _rtcVoltageLow = true;
  uint64_t _rtcBaseSeconds = 0;
  unsigned long _rtcBaseMillis = 0;
  long long _rtcLastPersistHost = 0;
  uint8_t _rtcRegisters[256]{};
};

extern TwoWire Wire;
