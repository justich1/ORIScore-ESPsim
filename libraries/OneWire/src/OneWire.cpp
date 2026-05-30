#include "OneWire.h"
#include <string.h>

OneWire::OneWire() {
  begin(255);
}

OneWire::OneWire(uint8_t pin) {
  begin(pin);
}

void OneWire::begin(uint8_t pin) {
  _pin = pin;
  memset(_selected, 0, sizeof(_selected));
  _lastCommand = 0;
  _readIndex = 0;
  _searchDone = false;

  // 45.0 C * 16 = 720
  _tempRaw = 720;

  makeDefaultRom();
}

void OneWire::makeDefaultRom() {
  _rom[0] = 0x28; // DS18B20 family
  _rom[1] = 0x00;
  _rom[2] = 0x00;
  _rom[3] = 0x00;
  _rom[4] = 0x00;
  _rom[5] = 0x00;
  _rom[6] = 0x01;
  _rom[7] = crc8(_rom, 7);
}

uint8_t OneWire::reset() {
  _readIndex = 0;
  return 1; // device present
}

void OneWire::select(const uint8_t rom[8]) {
  if (rom) {
    memcpy(_selected, rom, 8);
  }
}

void OneWire::skip() {
  memcpy(_selected, _rom, 8);
}

void OneWire::write(uint8_t v, uint8_t power) {
  (void)power;
  _lastCommand = v;
  _readIndex = 0;

  // 0x44 Convert T
  // 0xBE Read Scratchpad
}

uint8_t OneWire::read() {
  uint8_t sp[9];
  buildScratchpad(sp);

  if (_lastCommand == 0xBE) {
    if (_readIndex < 9) {
      return sp[_readIndex++];
    }
    return 0xFF;
  }

  return 0xFF;
}

void OneWire::reset_search() {
  _searchDone = false;
}

uint8_t OneWire::search(uint8_t* newAddr, bool search_mode) {
  (void)search_mode;

  if (_searchDone || !newAddr) {
    return 0;
  }

  memcpy(newAddr, _rom, 8);
  _searchDone = true;
  return 1;
}

void OneWire::buildScratchpad(uint8_t* sp) {
  memset(sp, 0, 9);

  sp[0] = (uint8_t)(_tempRaw & 0xFF);
  sp[1] = (uint8_t)((_tempRaw >> 8) & 0xFF);
  sp[2] = 0x4B; // TH
  sp[3] = 0x46; // TL
  sp[4] = 0x7F; // config 12bit
  sp[5] = 0xFF;
  sp[6] = 0x0C;
  sp[7] = 0x10;
  sp[8] = crc8(sp, 8);
}

uint8_t OneWire::crc8(const uint8_t* addr, uint8_t len) {
  uint8_t crc = 0;

  while (len--) {
    uint8_t inbyte = *addr++;

    for (uint8_t i = 8; i; i--) {
      uint8_t mix = (crc ^ inbyte) & 0x01;
      crc >>= 1;

      if (mix) {
        crc ^= 0x8C;
      }

      inbyte >>= 1;
    }
  }

  return crc;
}
