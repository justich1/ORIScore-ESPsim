#pragma once

/*
  EEPROM.h pro ORIScore ESPsim
  ----------------------------
  Minimalni kompatibilni nahrada ESP8266 EEPROM knihovny.

  Podporuje:
  - EEPROM.begin(size)
  - EEPROM.end()
  - EEPROM.read(address)
  - EEPROM.write(address, value)
  - EEPROM.update(address, value)
  - EEPROM.commit()
  - EEPROM.length()
  - EEPROM.get(address, data)
  - EEPROM.put(address, data)

  Ukladani:
  - zkusi devicefs/eeprom.bin
  - kdyz cesta nejde, zkusi eeprom.bin v aktualnim pracovnim adresari
*/

#include <Arduino.h>
#include <stdint.h>
#include <stddef.h>

#ifndef EEPROM_SIM_MAX_SIZE
#define EEPROM_SIM_MAX_SIZE 4096
#endif

class EEPROMClass {
public:
  EEPROMClass();

  bool begin(size_t size);
  void end();

  uint8_t read(int address);
  void write(int address, uint8_t value);
  void update(int address, uint8_t value);

  bool commit();
  size_t length();

  template<typename T>
  T& get(int address, T& data) {
    uint8_t* ptr = reinterpret_cast<uint8_t*>(&data);

    for (size_t i = 0; i < sizeof(T); i++) {
      ptr[i] = read(address + (int)i);
    }

    return data;
  }

  template<typename T>
  const T& put(int address, const T& data) {
    const uint8_t* ptr = reinterpret_cast<const uint8_t*>(&data);

    for (size_t i = 0; i < sizeof(T); i++) {
      update(address + (int)i, ptr[i]);
    }

    return data;
  }

private:
  bool validAddress(int address) const;
  bool loadFromFile();
  bool saveToFile();

  uint8_t _data[EEPROM_SIM_MAX_SIZE];
  size_t _size;
  bool _begun;
  bool _dirty;
};

extern EEPROMClass EEPROM;
