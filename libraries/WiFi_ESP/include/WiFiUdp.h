#pragma once

#include "Arduino.h"
#include "WiFi.h"
#include <cstdint>
#include <deque>
#include <vector>

class WiFiUDP {
public:
  WiFiUDP() = default;

  uint8_t begin(uint16_t port) {
    _localPort = port;
    Serial.println(String("[SIM] WiFiUDP.begin port=") + String((int)port));
    return 1;
  }

  static void stopAll() {
    Serial.println("[SIM] WiFiUDP.stopAll");
  }

  void stop() {
    Serial.println("[SIM] WiFiUDP.stop");
    _rx.clear();
    _tx.clear();
  }

  int beginPacket(const char* host, uint16_t port) {
    _remoteHost = String(host ? host : "");
    _remotePort = port;
    _tx.clear();

    Serial.println(
      String("[SIM] WiFiUDP.beginPacket host=") +
      _remoteHost +
      " port=" +
      String((int)port)
    );

    return 1;
  }

  int beginPacket(IPAddress ip, uint16_t port) {
    _remoteHost = ip.toString();
    _remotePort = port;
    _tx.clear();

    Serial.println(
      String("[SIM] WiFiUDP.beginPacket ip=") +
      _remoteHost +
      " port=" +
      String((int)port)
    );

    return 1;
  }

  int endPacket() {
    Serial.println(
      String("[SIM] WiFiUDP.endPacket bytes=") +
      String((int)_tx.size())
    );

    if (!_tx.empty()) {
      String payload;
      for (uint8_t b : _tx) payload += (char)b;
      Serial.println(String("[SIM] UDP payload: ") + payload);
    }

    _tx.clear();
    return 1;
  }

  size_t write(uint8_t b) {
    _tx.push_back(b);
    return 1;
  }

  size_t write(const uint8_t* buffer, size_t size) {
    if (!buffer) return 0;

    for (size_t i = 0; i < size; i++) {
      _tx.push_back(buffer[i]);
    }

    return size;
  }

  size_t write(const char* s) {
    if (!s) return 0;

    size_t n = 0;
    while (*s) {
      _tx.push_back((uint8_t)*s++);
      n++;
    }

    return n;
  }

  size_t write(const String& s) {
    return write(s.c_str());
  }

  size_t print(const String& s) { return write(s); }
  size_t print(const char* s) { return write(s); }
  size_t print(char c) { return write((uint8_t)c); }
  size_t print(int v) { return print(String(v)); }
  size_t print(unsigned int v) { return print(String(v)); }
  size_t print(long v) { return print(String(v)); }
  size_t print(unsigned long v) { return print(String(v)); }
  size_t print(float v) { return print(String(v)); }
  size_t print(double v) { return print(String(v)); }
  size_t print(IPAddress ip) { return print(ip.toString()); }

  size_t println() {
    write((uint8_t)'\r');
    write((uint8_t)'\n');
    return 2;
  }

  size_t println(const String& s) { size_t n = print(s); return n + println(); }
  size_t println(const char* s) { size_t n = print(s); return n + println(); }
  size_t println(char c) { size_t n = print(c); return n + println(); }
  size_t println(int v) { size_t n = print(v); return n + println(); }
  size_t println(unsigned int v) { size_t n = print(v); return n + println(); }
  size_t println(long v) { size_t n = print(v); return n + println(); }
  size_t println(unsigned long v) { size_t n = print(v); return n + println(); }
  size_t println(float v) { size_t n = print(v); return n + println(); }
  size_t println(double v) { size_t n = print(v); return n + println(); }
  size_t println(IPAddress ip) { size_t n = print(ip); return n + println(); }

  int parsePacket() {
    return (int)_rx.size();
  }

  int available() {
    return (int)_rx.size();
  }

  int read() {
    if (_rx.empty()) return -1;

    uint8_t b = _rx.front();
    _rx.pop_front();

    return b;
  }

  int read(unsigned char* buffer, size_t len) {
    if (!buffer) return 0;

    size_t n = 0;

    while (n < len && !_rx.empty()) {
      buffer[n++] = _rx.front();
      _rx.pop_front();
    }

    return (int)n;
  }

  int read(char* buffer, size_t len) {
    return read((unsigned char*)buffer, len);
  }

  int peek() {
    if (_rx.empty()) return -1;
    return _rx.front();
  }

  void flush() {
    _rx.clear();
  }

  IPAddress remoteIP() {
    return IPAddress();
  }

  uint16_t remotePort() {
    return _remotePort;
  }

  void injectRx(const uint8_t* data, size_t len) {
    if (!data) return;

    for (size_t i = 0; i < len; i++) {
      _rx.push_back(data[i]);
    }
  }

  void injectRx(const String& s) {
    for (size_t i = 0; i < s.length(); i++) {
      _rx.push_back((uint8_t)s[i]);
    }
  }

private:
  uint16_t _localPort = 0;
  uint16_t _remotePort = 0;
  String _remoteHost;
  std::vector<uint8_t> _tx;
  std::deque<uint8_t> _rx;
};

inline void stopAll() { WiFiUDP::stopAll(); }
