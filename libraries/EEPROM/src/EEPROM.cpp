#include "EEPROM.h"

#include <stdio.h>
#include <string.h>

#if defined(_WIN32)
  #include <direct.h>
  #define MKDIR(path) _mkdir(path)
#else
  #include <sys/stat.h>
  #include <sys/types.h>
  #define MKDIR(path) mkdir(path, 0777)
#endif

EEPROMClass EEPROM;

static const char* EEPROM_FILE_PATHS[] = {
  "devicefs/eeprom.bin",
  "./devicefs/eeprom.bin",
  "eeprom.bin",
  "./eeprom.bin"
};

static void ensureDirs() {
  MKDIR("devicefs");
}

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

  ensureDirs();
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

  if (_data[address] != value) {
    _data[address] = value;
    _dirty = true;
  }
}

void EEPROMClass::update(int address, uint8_t value) {
  write(address, value);
}

bool EEPROMClass::commit() {
  if (!_begun) {
    return false;
  }

  ensureDirs();

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
  for (size_t i = 0; i < sizeof(EEPROM_FILE_PATHS) / sizeof(EEPROM_FILE_PATHS[0]); i++) {
    FILE* f = fopen(EEPROM_FILE_PATHS[i], "rb");

    if (!f) {
      continue;
    }

    size_t rd = fread(_data, 1, _size, f);
    fclose(f);

    // Kdyz je soubor kratsi, zbytek zustane 0xFF.
    (void)rd;
    return true;
  }

  return false;
}

bool EEPROMClass::saveToFile() {
  for (size_t i = 0; i < sizeof(EEPROM_FILE_PATHS) / sizeof(EEPROM_FILE_PATHS[0]); i++) {
    FILE* f = fopen(EEPROM_FILE_PATHS[i], "wb");

    if (!f) {
      continue;
    }

    size_t written = fwrite(_data, 1, _size, f);
    fflush(f);
    fclose(f);

    return written == _size;
  }

  return false;
}
