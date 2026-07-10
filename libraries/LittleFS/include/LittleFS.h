#pragma once
#include "Arduino.h"

class File {
public:
  File();
  File(const String& path, const String& mode);
  operator bool() const;

  size_t write(uint8_t b);
  size_t write(const uint8_t* data, size_t len);

  size_t print(const String& s);
  size_t print(const char* s);

  String readString();
  int available();
  int read();

  size_t size();
  size_t readBytes(char* buffer, size_t length);
  size_t readBytes(uint8_t* buffer, size_t length);

  void close();
  String name() const;

private:
  String path;
  String mode;
  size_t readPos = 0;
  bool opened = false;
};

class FakeLittleFSClass {
public:
  bool begin(bool formatOnFail = false);
  bool format();
  bool exists(const String& path);
  File open(const String& path, const String& mode);
  void remove(const String& path);
  void setFile(const String& path, const String& content);
  String getFile(const String& path);
};

extern FakeLittleFSClass LittleFS;

// Compatibility: some ESP8266 sketches use SPIFFS without including SPIFFS.h.
// The object itself is defined in SPIFFS.cpp.
extern FakeLittleFSClass SPIFFS;

