#pragma once
#include "Arduino.h"
#include <map>
#include <vector>
#include <cstring>

class Preferences {
public:
  bool begin(const char* name, bool readOnly = false) {
    _name = name ? name : "";
    _readOnly = readOnly;
    return true;
  }

  void end() {}

  void clear() {
    _store.clear();
    _bytes.clear();
  }

  bool remove(const char* key) {
    String k = scopedKey(key);
    bool a = _store.erase(k) > 0;
    bool b = _bytes.erase(k) > 0;
    return a || b;
  }

  bool isKey(const char* key) const {
    String k = scopedKey(key);
    return _store.find(k) != _store.end() || _bytes.find(k) != _bytes.end();
  }

  size_t putString(const char* key, const String& value) {
    if (_readOnly) return 0;
    _store[scopedKey(key)] = value;
    return value.length();
  }

  size_t putString(const char* key, const char* value) {
    return putString(key, String(value));
  }

  String getString(const char* key, const String& defaultValue = String("")) const {
    auto it = _store.find(scopedKey(key));
    return it == _store.end() ? defaultValue : it->second;
  }

  size_t putBool(const char* key, bool value) {
    if (_readOnly) return 0;
    _store[scopedKey(key)] = value ? "1" : "0";
    return 1;
  }

  bool getBool(const char* key, bool defaultValue = false) const {
    auto it = _store.find(scopedKey(key));
    return it == _store.end() ? defaultValue : (it->second == "1" || it->second == "true" || it->second == "ON");
  }

  size_t putInt(const char* key, int32_t value) {
    if (_readOnly) return 0;
    _store[scopedKey(key)] = String((long)value);
    return 4;
  }

  int32_t getInt(const char* key, int32_t defaultValue = 0) const {
    auto it = _store.find(scopedKey(key));
    return it == _store.end() ? defaultValue : it->second.toInt();
  }

  size_t putUInt(const char* key, uint32_t value) {
    if (_readOnly) return 0;
    _store[scopedKey(key)] = String((unsigned long)value);
    return 4;
  }

  uint32_t getUInt(const char* key, uint32_t defaultValue = 0) const {
    auto it = _store.find(scopedKey(key));
    return it == _store.end() ? defaultValue : (uint32_t)it->second.toInt();
  }

  size_t putFloat(const char* key, float value) {
    if (_readOnly) return 0;
    _store[scopedKey(key)] = String(value, 6);
    return 4;
  }

  float getFloat(const char* key, float defaultValue = 0) const {
    auto it = _store.find(scopedKey(key));
    return it == _store.end() ? defaultValue : it->second.toFloat();
  }

  size_t putBytes(const char* key, const void* value, size_t len) {
    if (_readOnly || !value) return 0;
    const uint8_t* p = static_cast<const uint8_t*>(value);
    _bytes[scopedKey(key)] = std::vector<uint8_t>(p, p + len);
    return len;
  }

  size_t getBytes(const char* key, void* buf, size_t maxLen) const {
    if (!buf || maxLen == 0) return 0;
    auto it = _bytes.find(scopedKey(key));
    if (it == _bytes.end()) return 0;

    size_t n = it->second.size();
    if (n > maxLen) n = maxLen;

    std::memcpy(buf, it->second.data(), n);
    return n;
  }

  size_t getBytesLength(const char* key) const {
    auto it = _bytes.find(scopedKey(key));
    return it == _bytes.end() ? 0 : it->second.size();
  }

uint8_t getUChar(const char* key, uint8_t defaultValue = 0) {
    (void)key;
    return defaultValue;
}

size_t putUChar(const char* key, uint8_t value) {
    (void)key;
    (void)value;
    return 1;
}

private:
  String _name;
  bool _readOnly = false;

  // Statické mapy drží hodnoty i mezi různými instancemi Preferences.
  inline static std::map<String, String> _store;
  inline static std::map<String, std::vector<uint8_t>> _bytes;

  String scopedKey(const char* key) const {
    return _name + "/" + String(key ? key : "");
  }
};
