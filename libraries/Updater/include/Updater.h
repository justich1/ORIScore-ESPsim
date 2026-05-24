#pragma once
#include "Arduino.h"
class FakeUpdateClass {
public:
  bool begin(size_t size = 0) { (void)size; _error = false; return true; }
  size_t write(uint8_t* data, size_t len) { (void)data; return len; }
  size_t write(const uint8_t* data, size_t len) { (void)data; return len; }
  bool end(bool evenIfRemaining = false) { (void)evenIfRemaining; return !_error; }
  bool hasError() const { return _error; }
  String errorString() const { return _error ? String("Fake update error") : String(""); }
  int getError() const { return _error ? 1 : 0; }
  void printError(FakeSerial& out) { out.println(errorString()); }
private:
  bool _error = false;
};
extern FakeUpdateClass Update;
