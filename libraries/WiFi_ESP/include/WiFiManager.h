#pragma once

#include "Arduino.h"
#include "ESP8266WiFi.h"
#include <functional>
#include <vector>

/*
  ORIScore ESPsim fake WiFiManager

  Source-level simulator stub for tzapu/WiFiManager-style sketches.
  It does not create a real captive portal. It pretends that WiFi
  connection/config portal succeeds so firmware can continue running.
*/

#ifndef WM_DEBUG_LEVEL
#define WM_DEBUG_LEVEL 0
#endif

class WiFiManagerParameter {
public:
  WiFiManagerParameter() = default;

  WiFiManagerParameter(
    const char* custom
  ) {
    _customHtml = custom ? custom : "";
  }

  WiFiManagerParameter(
    const char* id,
    const char* placeholder,
    const char* defaultValue,
    int length
  ) {
    _id = id ? id : "";
    _placeholder = placeholder ? placeholder : "";
    _value = defaultValue ? defaultValue : "";
    _length = length;
  }

  WiFiManagerParameter(
    const char* id,
    const char* placeholder,
    const char* defaultValue,
    int length,
    const char* custom
  ) {
    _id = id ? id : "";
    _placeholder = placeholder ? placeholder : "";
    _value = defaultValue ? defaultValue : "";
    _length = length;
    _customHtml = custom ? custom : "";
  }

  WiFiManagerParameter(
    const char* id,
    const char* placeholder,
    const char* defaultValue,
    int length,
    const char* custom,
    int labelPlacement
  ) {
    (void)labelPlacement;
    _id = id ? id : "";
    _placeholder = placeholder ? placeholder : "";
    _value = defaultValue ? defaultValue : "";
    _length = length;
    _customHtml = custom ? custom : "";
  }

  const char* getID() const {
    return _id.c_str();
  }

  const char* getValue() const {
    return _value.c_str();
  }

  const char* getPlaceholder() const {
    return _placeholder.c_str();
  }

  const char* getCustomHTML() const {
    return _customHtml.c_str();
  }

  int getValueLength() const {
    return _length;
  }

  void setValue(const char* value, int length = -1) {
    _value = value ? value : "";
    if (length >= 0) _length = length;
  }

private:
  String _id;
  String _placeholder;
  String _value;
  String _customHtml;
  int _length = 0;
};

class WiFiManager {
public:
  using SaveConfigCallback = std::function<void()>;
  using ConfigModeCallback = std::function<void(WiFiManager*)>;

  WiFiManager() = default;

  void setDebugOutput(bool enabled) {
    _debug = enabled;
  }

  void setConfigPortalTimeout(unsigned long seconds) {
    _configPortalTimeout = seconds;
  }

  void setConnectTimeout(unsigned long seconds) {
    _connectTimeout = seconds;
  }

  void setTimeout(unsigned long seconds) {
    _configPortalTimeout = seconds;
  }

  void setConnectRetries(uint8_t retries) {
    _connectRetries = retries;
  }

  void setBreakAfterConfig(bool enabled) {
    _breakAfterConfig = enabled;
  }

  void setConfigPortalBlocking(bool enabled) {
    _configPortalBlocking = enabled;
  }

  void setSaveConfigCallback(SaveConfigCallback cb) {
    _saveConfigCallback = cb;
  }

  void setAPCallback(ConfigModeCallback cb) {
    _apCallback = cb;
  }

  void setPreSaveConfigCallback(SaveConfigCallback cb) {
    _preSaveConfigCallback = cb;
  }

  void setPostSaveConfigCallback(SaveConfigCallback cb) {
    _postSaveConfigCallback = cb;
  }

  void setClass(const char* cssClass) {
    _cssClass = cssClass ? cssClass : "";
  }

  void setTitle(const char* title) {
    _title = title ? title : "";
  }

  void setHostname(const char* hostname) {
    _hostname = hostname ? hostname : "";
  }

  void setMinimumSignalQuality(int quality = 8) {
    _minimumSignalQuality = quality;
  }

  void setShowStaticFields(bool enabled) {
    _showStaticFields = enabled;
  }

  void setShowDnsFields(bool enabled) {
    _showDnsFields = enabled;
  }

  void setShowInfoUpdate(bool enabled) {
    _showInfoUpdate = enabled;
  }

  void setDarkMode(bool enabled) {
    _darkMode = enabled;
  }

  void setAPStaticIPConfig(IPAddress ip, IPAddress gateway, IPAddress subnet) {
    _apIp = ip;
    _apGateway = gateway;
    _apSubnet = subnet;
  }

  void setSTAStaticIPConfig(IPAddress ip, IPAddress gateway, IPAddress subnet) {
    _staIp = ip;
    _staGateway = gateway;
    _staSubnet = subnet;
  }

  void setSTAStaticIPConfig(IPAddress ip, IPAddress gateway, IPAddress subnet, IPAddress dns) {
    _staIp = ip;
    _staGateway = gateway;
    _staSubnet = subnet;
    _staDns = dns;
  }

  bool addParameter(WiFiManagerParameter* parameter) {
    if (!parameter) return false;
    _params.push_back(parameter);
    return true;
  }

  bool autoConnect() {
    return autoConnect("ESPsim-AP", nullptr);
  }

  bool autoConnect(const char* apName) {
    return autoConnect(apName, nullptr);
  }

  bool autoConnect(const char* apName, const char* apPassword) {
    if (_debug) {
      Serial.println(String("[SIM] WiFiManager.autoConnect AP=") + String(apName ? apName : ""));
    }

    // Simulate that either stored WiFi exists or captive portal succeeds.
    WiFi.mode(WIFI_STA);
    WiFi.begin(_simSsid.c_str(), _simPass.c_str());

    if (_saveConfigCallback) {
      _saveConfigCallback();
    }

    return true;
  }

  bool startConfigPortal() {
    return startConfigPortal("ESPsim-AP", nullptr);
  }

  bool startConfigPortal(const char* apName) {
    return startConfigPortal(apName, nullptr);
  }

  bool startConfigPortal(const char* apName, const char* apPassword) {
    if (_debug) {
      Serial.println(String("[SIM] WiFiManager.startConfigPortal AP=") + String(apName ? apName : ""));
    }

    WiFi.mode(WIFI_AP_STA);
    WiFi.softAP(apName ? apName : "ESPsim-AP", apPassword);

    if (_apCallback) {
      _apCallback(this);
    }

    if (_preSaveConfigCallback) {
      _preSaveConfigCallback();
    }

    if (_saveConfigCallback) {
      _saveConfigCallback();
    }

    if (_postSaveConfigCallback) {
      _postSaveConfigCallback();
    }

    WiFi.mode(WIFI_STA);
    WiFi.begin(_simSsid.c_str(), _simPass.c_str());

    return true;
  }

  void resetSettings() {
    if (_debug) {
      Serial.println("[SIM] WiFiManager.resetSettings");
    }
    WiFi.disconnect(true);
  }

  bool getWiFiIsSaved() {
    return true;
  }

  bool isConfigPortalActive() const {
    return false;
  }

  bool getConfigPortalActive() const {
    return false;
  }

  void stopConfigPortal() {
    if (_debug) {
      Serial.println("[SIM] WiFiManager.stopConfigPortal");
    }
  }

  void process() {
    // Non-blocking WiFiManager loop hook. Nothing needed in simulator.
  }

  void reboot() {
    ESP.restart();
  }

  String getWiFiSSID(bool persistent = true) const {
    (void)persistent;
    return _simSsid;
  }

  String getWiFiPass(bool persistent = true) const {
    (void)persistent;
    return _simPass;
  }

  String getConfigPortalSSID() const {
    return "ESPsim-AP";
  }

  void erase() {
    resetSettings();
  }

private:
  bool _debug = false;
  bool _breakAfterConfig = false;
  bool _configPortalBlocking = true;
  bool _showStaticFields = true;
  bool _showDnsFields = true;
  bool _showInfoUpdate = true;
  bool _darkMode = false;

  unsigned long _configPortalTimeout = 0;
  unsigned long _connectTimeout = 0;
  uint8_t _connectRetries = 1;
  int _minimumSignalQuality = 8;

  String _cssClass;
  String _title;
  String _hostname;

  String _simSsid = "ESPsim-WiFi";
  String _simPass = "password";

  IPAddress _apIp;
  IPAddress _apGateway;
  IPAddress _apSubnet;

  IPAddress _staIp;
  IPAddress _staGateway;
  IPAddress _staSubnet;
  IPAddress _staDns;

  SaveConfigCallback _saveConfigCallback = nullptr;
  SaveConfigCallback _preSaveConfigCallback = nullptr;
  SaveConfigCallback _postSaveConfigCallback = nullptr;
  ConfigModeCallback _apCallback = nullptr;

  std::vector<WiFiManagerParameter*> _params;
};
