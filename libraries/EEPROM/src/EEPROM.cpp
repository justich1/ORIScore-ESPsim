#include "EEPROM.h"

#include <stdio.h>
#include <string.h>

EEPROMClass EEPROM;

static const char* EEPROM_FILE_PATH = "devicefs/eeprom.bin";

EEPROMClass::EEPROMClass() {
  memset(_data, 0xFF, sizeof(_data));
  _size = 0;
  _begun = false;
  _dirty = false;
}

bool EEPROMClass::begin(size_t size) {
  if (size == 0) {
    size = 512;
  }

  if (size > EEPROM_SIM_MAX_SIZE) {
    size = EEPROM_SIM_MAX_SIZE;
  }

  _size = size;
  _begun = true;
  _dirty = false;

  memset(_data, 0xFF, sizeof(_data));
  loadFromFile();

  return true;
}

void EEPROMClass::end() {
  if (_begun && _dirty) {
    commit();
  }

  _begun = false;
}

uint8_t EEPROMClass::read(int address) {
  if (!_begun || !validAddress(address)) {
    return 0xFF;
  }

  return _data[address];
}

void EEPROMClass::write(int address, uint8_t value) {
  if (!_begun || !validAddress(address)) {
    return;
  }

  _data[address] = value;
  _dirty = true;
}

void EEPROMClass::update(int address, uint8_t value) {
  if (!_begun || !validAddress(address)) {
    return;
  }

  if (_data[address] != value) {
    _data[address] = value;
    _dirty = true;
  }
}

bool EEPROMClass::commit() {
  if (!_begun) {
    return false;
  }

  bool ok = saveToFile();
  if (ok) {
    _dirty = false;
  }

  return ok;
}

size_t EEPROMClass::length() {
  return _size;
}

bool EEPROMClass::validAddress(int address) const {
  return address >= 0 && (size_t)address < _size;
}

bool EEPROMClass::loadFromFile() {
  FILE* f = fopen(EEPROM_FILE_PATH, "rb");
  if (!f) {
    return false;
  }

  fread(_data, 1, _size, f);
  fclose(f);
  return true;
}

bool EEPROMClass::saveToFile() {
  FILE* f = fopen(EEPROM_FILE_PATH, "wb");
  if (!f) {
    return false;
  }

  size_t written = fwrite(_data, 1, _size, f);
  fclose(f);

  return written == _size;
}
