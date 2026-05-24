#pragma once

#include "Arduino.h"
#include "WiFiUdp.h"

// Lightweight ORIScore ESPsim stub for arduino-libraries/NTPClient.
// It does not contact real NTP servers. It simulates time from millis()
// plus configurable offset.

#ifndef SEVENZYYEARS
#define SEVENZYYEARS 2208988800UL
#endif

class NTPClient {
public:
  NTPClient(WiFiUDP& udp)
    : _udp(&udp) {}

  NTPClient(WiFiUDP& udp, long timeOffset)
    : _udp(&udp), _timeOffset(timeOffset) {}

  NTPClient(WiFiUDP& udp, const char* poolServerName)
    : _udp(&udp), _poolServerName(poolServerName ? poolServerName : "pool.ntp.org") {}

  NTPClient(WiFiUDP& udp, const char* poolServerName, long timeOffset)
    : _udp(&udp),
      _poolServerName(poolServerName ? poolServerName : "pool.ntp.org"),
      _timeOffset(timeOffset) {}

  NTPClient(
    WiFiUDP& udp,
    const char* poolServerName,
    long timeOffset,
    unsigned long updateInterval
  )
    : _udp(&udp),
      _poolServerName(poolServerName ? poolServerName : "pool.ntp.org"),
      _timeOffset(timeOffset),
      _updateInterval(updateInterval) {}

  void begin() {
    _running = true;
    _lastUpdate = millis();
    _baseEpoch = _simDefaultEpoch;
    _timeSet = true;
    Serial.println(String("[SIM] NTPClient.begin server=") + _poolServerName);
  }

  void begin(uint16_t port) {
    _port = port;
    if (_udp) _udp->begin(port);
    begin();
  }

  void end() {
    _running = false;
    Serial.println("[SIM] NTPClient.end");
  }

  bool update() {
    if (!_running) begin();

    if (!_timeSet || (millis() - _lastUpdate) >= _updateInterval) {
      return forceUpdate();
    }

    return true;
  }

  bool forceUpdate() {
    _lastUpdate = millis();

    if (!_timeSet) {
      _baseEpoch = _simDefaultEpoch;
      _timeSet = true;
    }

    Serial.println(String("[SIM] NTPClient.forceUpdate -> ") + getFormattedTime());
    return true;
  }

  bool isTimeSet() const {
    return _timeSet;
  }

  unsigned long getEpochTime() const {
    unsigned long elapsed = millis() / 1000UL;
    return _baseEpoch + elapsed + (unsigned long)_timeOffset;
  }

  int getDay() const {
    // 0 = Sunday
    return (int)((getEpochTime() / 86400UL + 4UL) % 7UL);
  }

  int getHours() const {
    return (int)((getEpochTime() % 86400UL) / 3600UL);
  }

  int getMinutes() const {
    return (int)((getEpochTime() % 3600UL) / 60UL);
  }

  int getSeconds() const {
    return (int)(getEpochTime() % 60UL);
  }

  String getFormattedTime() const {
    char buf[16];
    snprintf(
      buf,
      sizeof(buf),
      "%02d:%02d:%02d",
      getHours(),
      getMinutes(),
      getSeconds()
    );
    return String(buf);
  }

  void setTimeOffset(long timeOffset) {
    _timeOffset = timeOffset;
  }

  void setUpdateInterval(unsigned long updateInterval) {
    _updateInterval = updateInterval;
  }

  void setPoolServerName(const char* poolServerName) {
    _poolServerName = poolServerName ? poolServerName : "pool.ntp.org";
  }

  // ESPsim helper: set simulated base epoch explicitly.
  // Example: timeClient.simSetEpoch(1710000000UL);
  void simSetEpoch(unsigned long epoch) {
    _baseEpoch = epoch;
    _lastUpdate = millis();
    _timeSet = true;
  }

private:
  WiFiUDP* _udp = nullptr;
  const char* _poolServerName = "pool.ntp.org";
  uint16_t _port = 1337;

  long _timeOffset = 0;
  unsigned long _updateInterval = 60000UL;
  unsigned long _lastUpdate = 0;
  unsigned long _baseEpoch = 1710000000UL;   // arbitrary stable fake timestamp
  unsigned long _simDefaultEpoch = 1710000000UL;

  bool _running = false;
  bool _timeSet = false;
};
