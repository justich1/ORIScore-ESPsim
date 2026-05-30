#pragma once

/*
  OneWire.h pro ORIScore ESPsim
  -----------------------------
  Kompatibilni nahrada / doplneni OneWire API pro DS18B20 kod.

  Doplnuje:
  - OneWire(uint8_t pin)
  - reset()
  - select(const uint8_t rom[8])
  - skip()
  - write(uint8_t v, uint8_t power = 0)
  - read()
  - reset_search()
  - search(uint8_t* newAddr, bool search_mode = true)
  - static crc8(const uint8_t* addr, uint8_t len)

  Pro simulaci vraci jeden DS18B20 ROM:
    28 00 00 00 00 00 01 CRC

  Hodnota teploty je defaultne 45.0 C.
  Pokud ORIS simulator umi vlastni DallasTemperature, tenhle OneWire bude porad
  kompatibilni i pro jednoduche rucni cteni scratchpadu.
*/

#include <Arduino.h>
#include <stdint.h>
#include <stddef.h>

#ifndef pgm_read_byte
#define pgm_read_byte(addr) (*(const uint8_t*)(addr))
#endif

#ifndef pgm_read_word
#define pgm_read_word(addr) (*(const uint16_t*)(addr))
#endif

#ifndef PROGMEM
#define PROGMEM
#endif

class OneWire {
public:
  OneWire(uint8_t pin);
  OneWire();

  void begin(uint8_t pin);

  uint8_t reset();
  void select(const uint8_t rom[8]);
  void skip();

  void write(uint8_t v, uint8_t power = 0);
  uint8_t read();

  void reset_search();
  uint8_t search(uint8_t* newAddr, bool search_mode = true);

  static uint8_t crc8(const uint8_t* addr, uint8_t len);

private:
  uint8_t _pin;
  uint8_t _selected[8];
  uint8_t _rom[8];

  uint8_t _lastCommand;
  uint8_t _readIndex;
  bool _searchDone;

  int16_t _tempRaw;

  void makeDefaultRom();
  void buildScratchpad(uint8_t* sp);
};
